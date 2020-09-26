#include "mpi.h"
#include <stdio.h>
#include <limits.h>

#define DIM 16

int main(int argc, char* argv[]){
    int worldrank, nproc;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int chunk=2;
    int rcvsize=DIM/nproc;
    int nChunks=rcvsize/chunk;

    if(worldrank==0){
        int V[DIM];
        FILE*f=fopen("v.txt","r");
        for(int i=0; i<DIM; i++){
                fscanf(f,"%d",&V[i]);            
        }

        MPI_Datatype chunksType;
        MPI_Type_vector(nChunks,chunk,nproc*chunk,MPI_INT,&chunksType);
        MPI_Type_commit(&chunksType);

        for(int i=0; i<nproc; i++){
            MPI_Send(&V[i*chunk],1,chunksType,i,0,MPI_COMM_WORLD);
        }
    }

    int Vlocal[rcvsize];
    MPI_Status status;
    MPI_Recv(Vlocal,rcvsize,MPI_INT,0,0,MPI_COMM_WORLD,&status); 

    printf("P%d, Vlocal:\n",worldrank);
    for(int i=0; i<rcvsize; i++){
        printf("%d ",Vlocal[i]);
    }
    printf("\n");

    /*
    Ordinamento decrescente
    */
   for(int i=0; i<rcvsize; i++){
       int max=Vlocal[i];
       int pos=i;
       for(int j=i+1; j<rcvsize; j++){
           if(Vlocal[j]>max){
               max=Vlocal[j];
               pos=j;
           }
       }

       //swap
       int tmp=Vlocal[i];
       Vlocal[i]=Vlocal[pos];
       Vlocal[pos]=tmp;
   }

   printf("P%d, Vlocal decrescente:\n",worldrank);
    for(int i=0; i<rcvsize; i++){
        printf("%d ",Vlocal[i]);
    }
    printf("\n");

    /*
    Nuovo ranking basato sui massimi
    */
   int localmax=Vlocal[0];
   for(int i=1; i<rcvsize; i++){
       if(Vlocal[i]>localmax) localmax=Vlocal[i];
   }

   int ranks[nproc];
   struct{
       int value; 
       int rank;
   } in, out;

   in.value=localmax;
   in.rank=worldrank;


   for(int i=0; i<nproc; i++){
       MPI_Allreduce(&in,&out,1,MPI_2INT,MPI_MAXLOC,MPI_COMM_WORLD);
       ranks[i]=out.rank;
       if(worldrank==out.rank){
           in.value=INT_MIN;
       }
   }

   if(worldrank==0){
       printf("Vettore dei ranghi:\n");
       for(int i=0; i<nproc; i++){
           printf("%d ",ranks[i]);
       }
   }

   MPI_Group group;
   MPI_Comm_group(MPI_COMM_WORLD,&group);
   MPI_Group_incl(group,nproc,ranks,&group);
   MPI_Comm newcomm;
   MPI_Comm_create(MPI_COMM_WORLD,group,&newcomm);
   int newrank;
   MPI_Comm_rank(newcomm,&newrank);

   printf("P%d, Vlocal nel newcomm:\n",newrank);
   for(int i=0; i<rcvsize; i++){
       printf("%d ",Vlocal[i]);
   }
   printf("\n");

   /*
    Shift
   */
  int source=(newrank-2 + nproc)%nproc;
  int dest=(newrank+2)%nproc;

  MPI_Sendrecv_replace(Vlocal,rcvsize,MPI_INT,dest,0,source,0,newcomm,&status);

  printf("P%d, dopo shift:\n",newrank);
  for(int i=0; i<rcvsize; i++){
      printf("%d ",Vlocal[i]);
  }
  printf("\n");

  /*
    C
  */
 int C[nproc][rcvsize];
 MPI_Gather(Vlocal,rcvsize,MPI_INT,C,rcvsize,MPI_INT,0,MPI_COMM_WORLD);

 if(worldrank==0){
     printf("P%d, C:\n",worldrank);
     for(int i=0; i<nproc; i++){
         for(int j=0; j<rcvsize; j++){
             printf("%d ",C[i][j]);
         }
         printf("\n");
     }
 }

 MPI_Finalize();

}