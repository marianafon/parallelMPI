#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void Read_n(int* n, int* local_n, int my_rank, int comm_sz, int* local_a, int* local_b, MPI_Comm comm);
void Allocate_vectors(int** local_vector_a, int local_n, MPI_Comm comm);
void Read_vector(int vector_a[], int local_n, int n, int my_rank, MPI_Comm comm);
void Print_vector(int local_vector_a[], int local_n, int n, char title[], int my_rank, MPI_Comm comm);
void My_MPI_Reduce(int comm_sz, int my_rank, int local_sum, int* result, MPI_Comm comm);

int main(void) {
   int n = 0, local_n = 0, local_sum = 0, total_sum = 0, result = 0, comm_sz = 0, my_rank = 0, local_a = 0, local_b = 0;
   int* local_vector_a; 
   MPI_Comm comm;
   
   /* Start up MPI */
   MPI_Init(NULL, NULL); 
   comm = MPI_COMM_WORLD;
   /* Get the number of processes */
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz); 
   /* Get my rank among all the processes */
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 

   /* 
    * Lê n (tem que ser potência de 2) e calcula local_n
    * a serem somados pelo core my_rank
    */
   Read_n(&n, &local_n, my_rank, comm_sz, &local_a, &local_b, comm);
   /* 
    * Envia em broadcast os valores que são necessários a todos os processos
    */
   MPI_Bcast(&local_n, 1, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
   /*
    * Aloca os vetores locais
    */
   Allocate_vectors(&local_vector_a, local_n, comm);
   /* 
    * Preenche os vetores e os distribui 
    */
   Read_vector(local_vector_a, local_n, n, my_rank, comm);
   /* 
    * Calcula a soma dos elementos do vetor local
    */
   int i = 0;
   for(i=0;i<local_n;i++){
      local_sum += local_vector_a[i];
   }

   //printf("=== My MPI Reduce === %d, %d, %d ===", my_rank, local_sum, local_n);
   /*
    *Adiciona todas as somas locais ao result com destino ao processo 0
    */
   MPI_Reduce(&local_sum, &result, 1, MPI_INT, MPI_SUM, 0, comm);
   My_MPI_Reduce(comm_sz, my_rank, local_sum, &total_sum, comm);

   MPI_Barrier(comm);
   /*
    *Imprime o resultado e libera a memória utilizada
    */
   if(my_rank == 0){
      printf("Result: %d | Total sum: %d\n",result, total_sum);
      free(local_vector_a);
   }

   /* Finaliza o MPI */
   MPI_Finalize(); 

   return 0;
}  /* main */

//---------------------------Functions------------------------------

void Read_n(int* n, int* local_n, int my_rank, int comm_sz, int* local_a, int* local_b, MPI_Comm comm)
{
   if (my_rank == 0) {
      printf("Qual o tamanho dos vetores?\n");
      scanf("%d", n);
      *local_n = *n/comm_sz;
      int resto = *n%comm_sz;

      if(my_rank < resto){
         *local_n += *local_n;
         *local_a = my_rank * (*local_n);      
         *local_b = *local_a + (*local_n);
      }else{
         *local_a = my_rank * (*local_n) + (resto);
         *local_b = *local_a + (*local_n);
      }
   }   
}

void Allocate_vectors(int** local_vector_a, int local_n, MPI_Comm comm)
{
   *local_vector_a = (int*)malloc(local_n*sizeof(int));
}
   
void Read_vector(int local_vector_a[], int local_n, int n, int my_rank, MPI_Comm comm)
{
   int i = 0;
   int* vector_a = NULL;


   if(my_rank == 0){
      vector_a = malloc(n*sizeof(int));

      printf("Defina o vetor (%d posições)\n", n);
      for(i=0;i<n;i++)
         scanf("%d", &vector_a[i]);
      
      MPI_Scatter(vector_a, local_n, MPI_INT, 
            local_vector_a, local_n, MPI_INT, 0, comm);
      
      free(vector_a);
   }else{
      MPI_Scatter(vector_a, local_n, MPI_INT, 
            local_vector_a, local_n, MPI_INT, 0, comm);
   }   
}

void My_MPI_Reduce(int comm_sz, int my_rank, int local_sum, int* result, MPI_Comm comm)
{
   int divisor = 2, core_difference = 1, from, to;
   int total_sum = local_sum;
   
   while(divisor <= comm_sz)
   {
      if(my_rank % divisor == 0)
      {
         if(my_rank + core_difference < comm_sz)
         {
            //Recebe a soma do core (my_rank + core_difference)
            //soma o total do core
            from = my_rank + core_difference;

            MPI_Recv(&local_sum, 1, MPI_INT, from, 0,
               comm, MPI_STATUS_IGNORE);
            total_sum += local_sum;

            printf("Sou o core %d, RECEBI %d do core %d\n",
                  my_rank, local_sum, from);   
         }
      }else{
         //Envia soma para o core (my_rank - core_difference)
         to = my_rank - core_difference;

         MPI_Send(&total_sum, 1, MPI_INT, to, 0, 
            comm);          

         printf("Sou o core %d, ENVIEI %d ao core %d\n",
               my_rank, total_sum, to);
         break;
      }
      divisor *= 2;
      core_difference *= 2;
   }

   *result = total_sum;
}