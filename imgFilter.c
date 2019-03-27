#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

typedef struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
}RGB;

typedef struct {
	int type;
	int width;
	int height;
	int maxVal;
	RGB **pixels;
}image;

void get_pic_size(const char *fileName, int *height, int *width) {
	FILE *input_file = fopen(fileName, "r");

	fseek(input_file, 2, SEEK_CUR);

	while(fgetc(input_file) != '\n');	

	int aux_height, aux_width;
	fscanf(input_file,"%d", &aux_width);
	fscanf(input_file,"%d", &aux_height);

	*height = aux_height;
	*width = aux_width;

	fclose(input_file);
}

void readData(const char *fileName, image *img) {
	int width, height, maxVal;

	FILE *input_file = fopen(fileName, "r");

	char ch = fgetc(input_file);
	ch = fgetc(input_file);
	int type = ch - 48;
	img->type = type;

	while(fgetc(input_file) != '\n');	

	fscanf(input_file,"%d", &width);
	fscanf(input_file,"%d", &height);
	fscanf(input_file,"%d", &maxVal);

	img->width = width;
	img->height = height;
	img->maxVal = maxVal;
	
	while(fgetc(input_file) != '\n');

	img->pixels = (RGB**)malloc(height * sizeof(RGB*));
	for(int i = 0; i < height; i++)
		img->pixels[i] = (RGB*)malloc(width * sizeof(RGB));

	/* Cazul in care imaginea e grayscale.
	Avand un singur canal de culoare, toate cele 3 campuri
	din structura RGB vor avea aceeasi valoare */
	if(type == 5) {
		for(int i = 0; i < height; i++) {
			for(int j = 0; j < width; j++) {

				ch = fgetc(input_file);
				img->pixels[i][j].red = ch;
				img->pixels[i][j].green = ch;
				img->pixels[i][j].blue = ch;
			}
		}
	
	/* Imaginea fiind color, fiecare camp din structura RGB va avea
	valoarea corespunzatoare canalului sau de culoare */
	} else {
		for(int i = 0; i < height; i++) {
			fread(img->pixels[i], sizeof(RGB), width, input_file);
		}
	}

	fclose(input_file);
}

void writeData(const char * fileName, image *img) {

	FILE *output_file = fopen(fileName, "w");

	char type[2];
	type[0] = 'P';
	type[1] = (img->type + 48);
	type[2] = '\0';

	fprintf(output_file, "%s\n", type);
	fprintf(output_file, "%d %d\n", img->width, img->height);
	fprintf(output_file, "%d\n", img->maxVal);

	if(img->type == 5) {
		for(int i = 0; i < img->height; i++) {
			for(int j = 0; j < img->width; j++) {
				fputc(img->pixels[i][j].red, output_file);
			}
		}

	} else {
		for(int i = 0; i < img->height; i++) {
			fwrite(img->pixels[i], sizeof(RGB), img->width, output_file);
		}
	}

	fclose(output_file);
}

void get_filter(const char* filter_name, float **filter) {

	if(strcmp(filter_name, "smooth") == 0) {

		for(int i = 0; i < 3; i++) {
			for(int j = 0; j < 3; j++) {
				filter[i][j] = (float)1/9;
			}
		}

	} else if(strcmp(filter_name, "blur") == 0) {
		
		for(int i = 0; i < 3; i++) {
			if(i % 2 == 0) {
				filter[i][0] = 1/(float)16;
				filter[i][1] = 1/(float)8;
				filter[i][2] = 1/(float)16;
			} else {
				filter[i][0] = 1/(float)8;
				filter[i][1] = 1/(float)4;
				filter[i][2] = 1/(float)8;
			}
		}

	} else if(strcmp(filter_name, "sharpen") == 0) {
		
		for(int i = 0; i < 3; i++) {
			if(i % 2 == 0) {
				filter[i][0] = 0;
				filter[i][1] = -2/(float)3;
				filter[i][2] = 0;
			} else {
				filter[i][0] = -2/(float)3;
				filter[i][1] = 11/(float)3;
				filter[i][2] = -2/(float)3;
			}
		}

	} else if(strcmp(filter_name, "mean") == 0) {
		
		for(int i = 0; i < 3; i++) {
			if(i % 2 == 0) {
				filter[i][0] = (float)-1;
				filter[i][1] = (float)-1;
				filter[i][2] = (float)-1;
			} else {
				filter[i][0] = (float)-1;
				filter[i][1] = (float)9;
				filter[i][2] = (float)-1;
			}
		}

	} else if(strcmp(filter_name, "emboss") == 0) {
		
		filter[0][0] = (float)0;
		filter[0][1] = (float)1;
		filter[0][2] = (float)0;

		filter[1][0] = (float)0;
		filter[1][1] = (float)0;
		filter[1][2] = (float)0;

		filter[2][0] = (float)0;
		filter[2][1] = (float)-1;
		filter[2][2] = (float)0;
	}
}

RGB filter_pixel(int line, int col, RGB **pixels, float **filter) {

	float red = 0, green = 0, blue = 0;
	RGB new_pixel;

	for(int i = 0, cur_line = line - 1; i < 3; i++, cur_line++) {
		for(int j = 0, cur_col = col - 1; j < 3; j++, cur_col++) {

			red += pixels[cur_line][cur_col].red * filter[i][j];
			green += pixels[cur_line][cur_col].green * filter[i][j];
			blue += pixels[cur_line][cur_col].blue * filter[i][j];
		}
	}

	new_pixel.red = (unsigned char)red;
	new_pixel.green = (unsigned char)green;
	new_pixel.blue = (unsigned char)blue;

	return new_pixel;
}

RGB **rgb_malloc(int lines, int cols) {

	RGB *data = malloc(lines * cols * sizeof(RGB));
	RGB **proc_pixels = (RGB**)malloc(lines * sizeof(RGB*));
	for(int i = 0; i < lines; i++)
		proc_pixels[i] = &(data[cols * i]);

	return proc_pixels;
}

int main(int argc, char** argv) {

	MPI_Init(&argc, &argv);

	int proc_size;
	MPI_Comm_size(MPI_COMM_WORLD, &proc_size);

	/* Am definit un nou MPI_Datatype pentru lucrul cu structura RGB */
    MPI_Datatype mpi_rgb;
    MPI_Type_contiguous(3, MPI_UNSIGNED_CHAR, &mpi_rgb);
    MPI_Type_commit(&mpi_rgb);

	int proc_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);

	int height, width;
	get_pic_size(argv[1], &height, &width);

	/* Numarul de linii ce trebuie prelucrate per proces. Daca numarul
	de linnii nu se imparte exact la numarul de procese, restul liniilor
	ramase vor fi prelucrate de ultimul proces */
	int proc_lines = height / proc_size;
	int rest_lines = height % proc_size;

	float **filter = malloc(3 * sizeof(float*));
	for(int i = 0; i < 3; i++)
		filter[i] = malloc(3 * sizeof(float));

	/* Procesul cu rank-ul 0 este responsabil de impartirea "bucatilor" din imagine
	catre celelalte procese in mod egal. La randul lui, acesta va procesa prima bucata 
	din imagine */
	if(proc_rank == 0) {

		image *img = (image*)malloc(sizeof(image));
		readData(argv[1], img);

		for(int filter_no = 3; filter_no < argc; filter_no++) {

			//Trimite date catre celelalte procese
			for(int proc_no = 1; proc_no < proc_size; proc_no++) {

				RGB **proc_pixels;
				int range, limit;

				/* Pe langa liniile propriu-zise ce trebuie procesate, fiecarui proces
				i se vor mai trimite 2 linii in plus: marginea de sus si cea de jos, 
				pentru aplicarea corecta a filtrului. Totusi, ultimului proces (responsabil
				de prelucrarea ultimei bucati din imagine) i se va trimite doar marginea
				superioara, deoarece, conform enuntului, prima si ultima linie de pixeli
				din imagine raman neprelucrate . De asemenea, procesul 0 nu va prelucra
				prima linie din bucata sa, deoarece aceasta coincide cu prima linie de pixeli
				a pozei . Variabila range desemneaza aceste marimi prezentate mai sus*/

				if(proc_no != proc_size - 1){
					range = proc_lines + 2;
					limit = (proc_no + 1) * proc_lines + 1;
				}
				else{
					range = proc_lines + 1 + rest_lines;
					limit = (proc_no + 1) * proc_lines + rest_lines;
				}

				/* Datele (liniile cu pixeli din imagine, "bucatile din imagine") sunt
				stocate in buffer-ul proc_pixels si trimise mai apoi catre fiecare proces */
				proc_pixels = rgb_malloc(range, img->width);

				for(int i = proc_lines * proc_no - 1, j = 0; i < limit; i++, j++)
					memcpy(proc_pixels[j], img->pixels[i], img->width * sizeof(RGB));

				MPI_Send(&(proc_pixels[0][0]), range * img->width, mpi_rgb, proc_no, proc_no, MPI_COMM_WORLD);

				free(proc_pixels[0]);
				free(proc_pixels);
			}

			//Aplicarea filtrului de catre procesul 0

			get_filter(argv[filter_no], filter);

			RGB **new_pixels = rgb_malloc(proc_lines, img->width);

			memcpy(new_pixels[0], img->pixels[0], img->width * sizeof(RGB));

			/* In mod normal, procesul cu rank 0 ar trebuie sa proceseze bucata sa din imagine
			pana la ultima linie (din bucata respectiva). Totusi, in cazul in care exista
			doar un singur proces, acesat "bucata" va deveni imaginea intreaga. Astfel,
			ultima linie nu trebuie prelucrata. */
			int limit;
			if(proc_size == 1) {
				limit = proc_lines - 1;
				memcpy(new_pixels[limit], img->pixels[limit], img->width * sizeof(RGB));
			
			} else {
				limit = proc_lines;
			}

			for(int line = 1; line < limit; line++) {
				for(int col = 0; col < img->width; col++) {

					/* Primul si ultimul pixel din fiecare coloana nu trebuie prelucrati
					deoarece acestia fac parte din marginile stanga-dreapta ale imaginii */
					if(col == 0 || col == img->width - 1) {
						new_pixels[line][col] = img->pixels[line][col];

					} else {
						new_pixels[line][col] = filter_pixel(line, col, img->pixels, filter);
					}
				}
			}

			for(int i = 0; i < proc_lines; i++)
				memcpy(img->pixels[i], new_pixels[i], img->width * sizeof(RGB));

			free(new_pixels[0]);
			free(new_pixels);

			//Primeste datele de la celelalte procese
			
			for(int proc_no = 1; proc_no < proc_size; proc_no++) {

				int range, limit;

				if(proc_no == proc_size - 1) {

					range = proc_lines + rest_lines;
					limit = (proc_no + 1) * proc_lines + rest_lines;

				} else {

					range = proc_lines;
					limit = (proc_no + 1) * proc_lines;
				}

				RGB **proc_pixels = rgb_malloc(range, img->width);

				MPI_Recv(&(proc_pixels[0][0]), range * img->width, mpi_rgb, proc_no, proc_no, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				for(int i = proc_no * proc_lines, j = 0 ; i < limit; i++, j++)
					memcpy(img->pixels[i], proc_pixels[j], img->width * sizeof(RGB));

				free(proc_pixels[0]);
				free(proc_pixels);
			}
		}

		writeData(argv[2], img);


	/* Ultimul proces isi va prelucra bucata din imagine pana la penultimul rand
	de pixeli inclusiv */
	} else if(proc_rank == proc_size - 1) {

		for(int filter_no = 3; filter_no < argc; filter_no++) {

			int range = proc_lines + 1 + rest_lines;

			/* Initializeaza buffer-ul unde vor fi stocate datele primite*/
			RGB **proc_pixels = rgb_malloc(range, width);

			/* Asteapta sa primeasca datele */
			MPI_Recv(&(proc_pixels[0][0]), range * width, mpi_rgb, 0, proc_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
			get_filter(argv[filter_no], filter);

			/* Initializeaza zona de memorie unde vor fi stocati pixelii filtrati */
			RGB **new_pixels = rgb_malloc(proc_lines + rest_lines, width);

			/* Prelucreaza datele */
			memcpy(new_pixels[range - 2], proc_pixels[range - 1], width * sizeof(RGB));

			for(int line = 1; line < proc_lines + rest_lines; line++) {
				for(int col = 0; col < width; col++) {

					if(col == 0 || col == width - 1) {
						new_pixels[line - 1][col] = proc_pixels[line][col];

					} else {
						new_pixels[line - 1][col] = filter_pixel(line, col, proc_pixels, filter);
					}
				}
			}

			/* Elibereaza buffer-ul de primire al datelor */
			free(proc_pixels[0]);
			free(proc_pixels);

			/* Trimite datele prelucrate */
			MPI_Send(&(new_pixels[0][0]), (proc_lines + rest_lines) * width, mpi_rgb, 0, proc_rank, MPI_COMM_WORLD);

			free(new_pixels[0]);
			free(new_pixels);
		}

	} else {

		for(int filter_no = 3; filter_no < argc; filter_no++) {

			int range = proc_lines + 2;

			/* Initializeaza buffer-ul unde vor fi stocate datele primite*/
			RGB **proc_pixels = rgb_malloc(range, width);

			/* Asteapta sa primeasca datele */
			MPI_Recv(&(proc_pixels[0][0]), range * width, mpi_rgb, 0, proc_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			get_filter(argv[filter_no], filter);

			/* Initializeaza zona de memorie unde vor fi stocati pixelii filtrati */
			RGB **new_pixels = rgb_malloc(proc_lines, width);

			/* Prelucreaza datele */
			for(int line = 1; line < proc_lines + 1; line++) {
				for(int col = 0; col < width; col++) {

					if(col == 0 || col == width - 1) {
						new_pixels[line - 1][col] = proc_pixels[line][col];

					} else {
						new_pixels[line - 1][col] = filter_pixel(line, col, proc_pixels, filter);
					}
				}
			}

			/* Elibereaza buffer-ul de primire al datelor */
			free(proc_pixels[0]);
			free(proc_pixels);

			/* Trimite datele prelucrate */
			MPI_Send(&(new_pixels[0][0]), proc_lines * width, mpi_rgb, 0, proc_rank, MPI_COMM_WORLD);

			free(new_pixels[0]);
			free(new_pixels);
		}
	}

	for(int i = 0; i < 3; i++)
		free(filter[i]);
	free(filter);

	MPI_Type_free(&mpi_rgb);
	MPI_Finalize();

	return 0;
}