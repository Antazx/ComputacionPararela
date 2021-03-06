/*
* Contar cuerpos celestes
*
* Asignatura Computación Paralela (Grado Ingeniería Informática)
* Código secuencial base
*
* @author Ana Moretón Fernández
* @version v1.2
*
* (c) 2017, Grupo Trasgo, Universidad de Valladolid
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "cputils.h"
#include <omp.h>



/* Substituir min por el operador */
#define min(x,y)    ((x) < (y)? (x) : (y))

/**
* Funcion principal
*/
int main (int argc, char* argv[])
{

	/* 1. Leer argumento y declaraciones */
	if (argc < 2) 	{ 		
		printf("Uso: %s <imagen_a_procesar>\n", argv[0]); 		
		return(EXIT_SUCCESS); 	
	} 	
	char* image_filename = argv[1]; 	

	int rows=-1;
	int columns =-1; 
	int *matrixData=NULL; 
	int *matrixResult=NULL; 
	int *matrixResultCopy=NULL; 
	int numBlocks=-1;



	/* 2. Leer Fichero de entrada e inicializar datos */

	/* 2.1 Abrir fichero */
	FILE *f = cp_abrir_fichero(image_filename);

	// Compruebo que no ha habido errores
	if (f==NULL)
	{
	   perror ("Error al abrir fichero.txt");
	   return -1;
	}

	/* 2.2 Leo valores del fichero */
	int i,j,valor;
	fscanf (f, "%d\n", &rows);
	fscanf (f, "%d\n", &columns);
	// Añado dos filas y dos columnas mas para los bordes
	rows=rows+2;
	columns = columns+2; 

	/* 2.3 Reservo la memoria necesaria para la matriz de datos */
	matrixData= (int *)malloc( rows*(columns) * sizeof(int) );
	if ( (matrixData == NULL)   ) {
 		perror ("Error reservando memoria");
	   	return -1;
	}

	/* 2.4 Inicializo matrices */
	for(i=0;i< rows; i++){
		for(j=0;j< columns; j++){
			matrixData[i*(columns)+j]=-1;
		}
	}
	/* 2.5 Relleno bordes de la matriz */
	for(i=1;i<rows-1;i++){
		matrixData[i*(columns)+0]=0;
		matrixData[i*(columns)+columns-1]=0;
	}
	for(i=1;i<columns-1;i++){
		matrixData[0*(columns)+i]=0;
		matrixData[(columns-1)*(columns)+i]=0;
	}
	/* 2.6 Relleno la matriz con los datos del fichero */
	for(i=1;i<rows-1;i++){
		for(j=1;j<columns-1;j++){
			fscanf (f, "%d\n", &matrixData[i*(columns)+j]);
		}
	}
	fclose(f);

	#ifdef WRITE
		printf("Inicializacion \n");
		for(i=0;i<rows;i++){
			for(j=0;j<columns;j++){
				printf ("%d\t", matrixData[i*(columns)+j]);
			}
			printf("\n");
		}
	#endif


	/* PUNTO DE INICIO MEDIDA DE TIEMPO */
	double t_ini = cp_Wtime();

//
// EL CODIGO A PARALELIZAR COMIENZA AQUI
//

	/* 3. Etiquetado inicial */



	matrixResult= (int *)malloc( (rows)*(columns) * sizeof(int) );
	matrixResultCopy= (int *)malloc( (rows)*(columns) * sizeof(int) );
	if ( (matrixResult == NULL)  || (matrixResultCopy == NULL)  ) {
 		perror ("Error reservando memoria");
	   	return -1;
	
	}

	#pragma omp parallel for shared(matrixResult, matrixData), private(i,j)
	for(i=0;i< rows; i++){
		for(j=0;j< columns; j++){
			matrixResult[i*(columns)+j]=-1;
			// Si es 0 se trata del fondo y no lo computamos
			if(matrixData[i*(columns)+j]!=0){
				matrixResult[i*(columns)+j]=i*(columns)+j;
			}
		}
	}



	/* 4. Computacion */
	int t=0;
	/* 4.1 Flag para ver si ha habido cambios y si se continua la ejecucion */
	int flagCambio=1; 
	int tmp = 0;

	/* 4.2 Busqueda de los bloques similiares */
	for(t=0; flagCambio !=0; t++){
		flagCambio=0; 

		/* 4.2.1 Actualizacion copia */
		#pragma omp parallel for shared(matrixResult, matrixResultCopy), private(i,j)
		for(i=1;i<rows-1;i++){
			for(j=1;j<columns-1;j++){
				if(matrixResult[i*(columns)+j]!=-1){
					matrixResultCopy[i*(columns)+j]=matrixResult[i*(columns)+j];
				}
			}
		}

		/* 4.2.2 Computo y detecto si ha habido cambios */
		#pragma omp parallel for shared(matrixData, matrixResult, matrixResultCopy), private (i, j), reduction(+:flagCambio)
		for(i=1;i<rows-1;i++){
			for(j=1;j<columns-1;j++){
				

				int tmp;
				int result=matrixResultCopy[i*columns+j];
				
				{
					if( result!= -1){
						//Si es de mi mismo grupo, entonces actualizo
						if(matrixData[(i-1)*columns+j] == matrixData[i*columns+j])
						{	
							result = min (result, matrixResultCopy[(i-1)*columns+j]);
						}
						if(matrixData[(i+1)*columns+j] == matrixData[i*columns+j])
						{
							result = min (result, matrixResultCopy[(i+1)*columns+j]);
						}
						if(matrixData[i*columns+j-1] == matrixData[i*columns+j])
						{
							result = min (result, matrixResultCopy[i*columns+j-1]);
						}
						if(matrixData[i*columns+j+1] == matrixData[i*columns+j])
						{
							result = min (result, matrixResultCopy[i*columns+j+1]);
						}

						// Si el indice no ha cambiado retorna 0
						if(matrixResult[i*columns+j] == result){ tmp = 0; }
						// Si el indice cambia, actualizo matrix de resultados con el indice adecuado y retorno 1
						else { matrixResult[i*columns+j]=result; tmp = 1;}
		
					}else{ 
						tmp = 0;
						
					}
				flagCambio= flagCambio+ tmp;
			}

		}	
	}

		#ifdef DEBUG
			printf("\nResultados iter %d: \n", t);
			for(i=0;i<rows;i++){
				for(j=0;j<columns;j++){
					printf ("%d\t", matrixResult[i*columns+j]);
				}
				printf("\n");
			}
		#endif

	}

	/* 4.3 Inicio cuenta del numero de bloques */
	
	numBlocks=0;
	#pragma omp parallel for shared(matrixResult), private(i,j)
	for(i=1;i<rows-1;i++){
		for(j=1;j<columns-1;j++){
			if(matrixResult[i*columns+j] == i*columns+j){
				#pragma omp atomic
				 numBlocks++;
			} 
		}
	}

//
// EL CODIGO A PARALELIZAR TERMINA AQUI
//

	/* PUNTO DE FINAL DE MEDIDA DE TIEMPO */
 	double t_fin = cp_Wtime();



	/* 5. Comprobación de resultados */
  	double t_total = (double)(t_fin - t_ini);

	printf("Result: %d\n", numBlocks);
	printf("Time: %lf\n", t_total);
	#ifdef WRITE
		printf("Resultado: \n");
		for(i=0;i<rows;i++){
			for(j=0;j<columns;j++){
				printf ("%d\t", matrixResult[i*columns+j]);
			}
			printf("\n");
		}
	#endif

	/* 6. Liberacion de memoria */
	free(matrixData); 
	free(matrixResult);
	free(matrixResultCopy); 

}
