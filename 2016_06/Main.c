#include "mpi.h"
#include <stdio.h>
#include <limits.h>
#include <malloc.h>

#define dim 20

int main(int argc, char* argv[]){
    int worldrank, nproc;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&worldrank);
    MPI_Comm_size(MPI_COMM_WORLD,&nproc);

    int k=2;
    int V[dim/nproc];
   

    if(worldrank==0){
         
        int A[dim];
        FILE*f=fopen("v.txt","r");
        for(int i=0; i<dim; i++){
            fscanf(f,"%d",&A[i]);
        }


        printf("P%d A:\n",worldrank);
        for(int i=0; i<dim; i++){
            printf("%d ",A[i]);
        }
        printf("\n");

        MPI_Datatype chunksType;
        MPI_Type_vector( (dim/nproc)/k,k,nproc*k,MPI_INT,&chunksType);
        MPI_Type_commit(&chunksType);

        for(int i=0; i<nproc; i++){
            MPI_Send(&A[i*k],1,chunksType,i,0,MPI_COMM_WORLD);
        }
    }

    MPI_Status status;
    MPI_Recv(V,dim/nproc,MPI_INT,0,0,MPI_COMM_WORLD,&status);
    printf("P%d, V:\n",worldrank);
    for(int i=0; i<dim/nproc; i++){
        printf("%d ", V[i]);
    }

    /*
    Ordinamento crescente
    */
   for(int i=0; i<dim/nproc; i++){
       for(int j=i+1; j<dim/nproc; j++){
           if(V[i]>V[j]){
               int tmp=V[i];
               V[i]=V[j];
               V[j]=tmp;
           }
       }
   }
    printf("\nP%d, V ordinato:\n",worldrank);
    for(int i=0; i<dim/nproc; i++){
        printf("%d ", V[i]);
    }

   /*
    Creazione gruppi
   */
  
  int gruppo=0;
  if(V[(dim/nproc)-1]>=0){
      gruppo=0;
  }
  else gruppo=1;

  MPI_Comm newcomm;
  MPI_Comm_split(MPI_COMM_WORLD,gruppo,worldrank,&newcomm);
  int newrank;
  MPI_Comm_rank(newcomm,&newrank);

  /*
    Somma del max del V del primo gruppo con il min del secondo gruppo
  */
 
 if(gruppo==0){
     int max;
     int maxlocal=INT_MIN;
     for(int i=0; i<dim/nproc; i++){
         if(V[i]>maxlocal) maxlocal=V[i];
     }
     MPI_Reduce(&maxlocal,&max,1,MPI_INT,MPI_MAX,0,newcomm);
     if(newrank==0){
         MPI_Send(&max,1,MPI_INT,0,1,MPI_COMM_WORLD);
     }
 }
 else{
     int min;
     int minlocal=INT_MAX;
     for(int i=0; i<dim/nproc; i++){
         if(V[i]<minlocal) minlocal=V[i];
     }
     MPI_Reduce(&minlocal,&min,1,MPI_INT,MPI_MIN,0,newcomm);
     if(newrank==0){
         MPI_Send(&min,1,MPI_INT,0,2,MPI_COMM_WORLD);
     }
 }

 if(worldrank==0){
     int max, min, somma;
     MPI_Status status;
     MPI_Recv(&max,1,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,&status);
     MPI_Recv(&min,1,MPI_INT,MPI_ANY_SOURCE,2,MPI_COMM_WORLD,&status);
     somma=max+min;
     printf("\nP%d, max=%d, min=%d, somma=%d\n",worldrank,max,min,somma);
 }

 MPI_Finalize();

}