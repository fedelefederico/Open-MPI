#include "mpi.h"
#include <stdio.h>

#define p 9
#define k 5

int main(int argc, char *argv[])
{
    int worldrank, nproc;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    int A[p][k];
    int righe = p / nproc;
    int Alocal[righe][k];

    if (worldrank == 0)
    {
        FILE *f = fopen("m.txt", "r");
        for (int i = 0; i < p; i++)
        {
            for (int j = 0; j < k; j++)
            {
                fscanf(f, "%d", &A[i][j]);
            }
        }
    }

    MPI_Scatter(A, righe * k, MPI_INT, Alocal, righe * k, MPI_INT, 0, MPI_COMM_WORLD);
    printf("P%d, Alocal:\n", worldrank);
    for (int i = 0; i < righe; i++)
    {
        for (int j = 0; j < k; j++)
        {
            printf("%d ", Alocal[i][j]);
        }
        printf("\n");
    }

    /*
    Calcolo Alocal con nuove righe
    */
    for (int i = 1; i < righe; i++)
    {
        for (int j = 0; j < k; j++)
        {
            Alocal[i][j] = Alocal[i][j] + Alocal[i - 1][j];
        }
    }

    printf("\nP%d, A new rows:\n", worldrank);
    for (int i = 0; i < righe; i++)
    {
        for (int j = 0; j < k; j++)
        {
            printf("%d,", Alocal[i][j]);
        }
        printf("\n");
    }

    //Vettore minimi righe
    int minimi[righe];
    for (int i = 0; i < righe; i++)
    {
        int minRiga = Alocal[i][0];
        for (int j = 1; j < k; j++)
        {
            if (Alocal[i][j] < minRiga)
                minRiga = Alocal[i][j];
        }
        minimi[i] = minRiga;
    }

    printf("P%d, minimi:\n", worldrank);
    for (int i = 0; i < righe; i++)
    {
        printf("%d ", minimi[i]);
    }

    //Vettore massimi colonne
    int massimi[k];
    for (int j = 0; j < k; j++)
    {
        int maxColonna = Alocal[0][j];
        for (int i = 1; i < righe; i++)
        {
            if (Alocal[i][j] > maxColonna)
                maxColonna = Alocal[i][j];
        }
        massimi[j] = maxColonna;
    }

    printf("\nP%d, massimi:\n", worldrank);
    for (int i = 0; i < k; i++)
    {
        printf("%d ", massimi[i]);
    }

    /*
    MINR e MAXC
   */
    int MINR[righe], MAXC[k];
    MPI_Reduce(minimi, MINR, righe, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(massimi, MAXC, k, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    if (worldrank == 0){
        printf("\nP%d, MINR:\n", worldrank);
        for (int i = 0; i < righe; i++)
        {
            printf("%d<", MINR[i]);
        }
        printf("\nP%d, MAXC\n", worldrank);
        for (int i = 0; i < k; i++)
        {
            printf("%d>", MAXC[i]);
        }

        FILE*f=fopen("MINR.txt","w");
        for(int i=0; i<righe; i++){
            fprintf(f,"%d ",MINR[i]);
        }
        FILE*f2=fopen("MAXC.txt","w");
        for(int i=0; i<k; i++){
            fprintf(f2,"%d ",MAXC[i]);
        }
    }

    MPI_Finalize();
}