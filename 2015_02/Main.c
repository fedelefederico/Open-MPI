#include "mpi.h"
#include <stdio.h>

#define DIM 6

int main(int argc, char* argv[]){
    int worldrank,nproc;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int A[DIM][DIM];

    if(worldrank==0){        
        FILE*f=fopen("M1.txt","r");
        for(int i=0; i<DIM; i++){
            for(int j=0; j<DIM; j++){
                fscanf(f,"%d",&A[i][j]);
            }
        }

        printf("P%d, A:\n",worldrank);
        for(int i=0; i<DIM; i++){
            for(int j=0; j<DIM; j++){
                printf("%d ",A[i][j]);
            }
            printf("\n");
        }
    }

    MPI_Datatype chunksType;
    MPI_Type_vector(DIM/2,DIM/(nproc/2),DIM,MPI_INT,&chunksType);
    MPI_Type_create_resized(chunksType,0,sizeof(int),&chunksType);
    MPI_Type_commit(&chunksType);

    int Alocal[DIM/2][DIM/(nproc/2)];
    int sendcounts[nproc];
    for(int i=0; i<nproc; i++){
        sendcounts[i]=1;
    }
    int displs[nproc];
    int index=0;
    for(int i=0; i<2; i++){
        for(int j=0; j<(nproc/2);j++){
            displs[index]=i*(DIM/2)*DIM+j*DIM/(nproc/2);
            index++;
        }
    }
    MPI_Scatterv(A,sendcounts,displs,chunksType,Alocal,(DIM/2)*(DIM/(nproc/2)),MPI_INT,0,MPI_COMM_WORLD);


    printf("P%d, Alocal:\n",worldrank);
    for(int i=0; i<DIM/2; i++){
        for(int j=0; j<DIM/(nproc/2); j++){
            printf("%d ",Alocal[i][j]);
        }
        printf("\n");
    }

    /*
    Shift
    */

    int color=0;
    if(worldrank>=nproc/2) color=1;

    MPI_Comm newcomm;
    MPI_Comm_split(MPI_COMM_WORLD,color,worldrank,&newcomm);
    int newrank;
    int comm_size;
    MPI_Comm_size(newcomm,&comm_size);
    MPI_Comm_rank(newcomm,&newrank);
    int source=(newrank-1+comm_size)%comm_size;
    int dest=(newrank+1)%comm_size;

    MPI_Status status;
    MPI_Sendrecv_replace(Alocal,(DIM/2)*(DIM/(nproc/2)),MPI_INT,dest,0,source,0,newcomm,&status);

    printf("P%d, shift:\n",worldrank);
    for(int i=0; i<DIM/2; i++){
        for(int j=0; j<DIM/(nproc/2); j++){
            printf("%d ",Alocal[i][j]);
        }
        printf("\n");
    }

    /*
    minimo prima colonna
    */
   int min=Alocal[0][0];
   for(int i=1; i<DIM/2; i++){
       if(Alocal[i][0]<min) min=Alocal[i][0];
   }

   struct{
       int value;
       int rank;
   } in, out;

   MPI_Allreduce(&in,&out,1,MPI_2INT,MPI_MAXLOC,newcomm);

   int vincitore=0;
   if(newrank==out.rank){
       vincitore=1;
   }

   MPI_Comm vincitori_comm;
   MPI_Comm_split(newcomm,vincitore,newrank,&vincitori_comm);
   int vincitori_rank;
   MPI_Comm_rank(vincitori_comm,&vincitori_rank);

   int minglobale;
   MPI_Reduce(&out.value,&minglobale,1,MPI_INT,MPI_MIN,0,vincitori_comm);
   if(vincitori_rank==0){
       MPI_Send(&minglobale,1,MPI_INT,0,0,MPI_COMM_WORLD);
   }

   if(worldrank==0){
       int min_rcv;
       MPI_Recv(&min_rcv,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
       printf("P%d, minimo globale: %d\n",worldrank,min_rcv);
   }

    MPI_Finalize();
}

