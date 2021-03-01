// File in inc_ml, created by Thien Le in July 2018
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "utilities.h"
#include "tools.h"
#include "quartet.h"

char STOCK_MSA_NAME[] = "tmp.msa";
char STOCK_QUARTET_NAME[] = "tmp.quartet";

int match_and_print_msa(char * tmp, char * aln, char ** name_map, int * u);
int make_quartet(DIST_MOD distance_model, char * tmp_folder);
int process_quartet(char * tmp, char ** name_map, int * u, int * res);
int do_quartet_ll(
    char ** name_map, 
    int * u, 
    DIST_MOD distance_model, 
    char * tmp_folder,
    int * res, 
    double * M);

// Implementation of FPM. Input is a distance matrix and a quartet, u1 - u3 are 
//    known leaves, x is query taxon, results shows which leave is the sibling 
//    of the query taxon
int four_point_method(float ** d, int * u, int * res){ // GC: added * to u 
  int i;
  float tmp, m = 1e9;

  for(i = 0; i < 3; i++){
    tmp = d[u[i]][u[3]] + ((i == 1) ? d[u[0]][u[2]] : d[u[1]][u[2 - i]]);
    if(m > tmp){
      m = tmp;
      *res = i;
    }
  }

  return 0;
}

int new_quartets_raxml(
    char ** name_map, 
    int * u, 
    int * res, 
    ml_options * master_ml_options)
{
  FCAL(
      GENERAL_ERROR,
      F_MATCH_PRINT_IN_NEW_Q_RXML,
      match_and_print_msa(master_ml_options->tmp_folder, master_ml_options->input_alignment, name_map, u) 
  ); 
  FCAL(
      GENERAL_ERROR,
      F_MK_Q_IN_NEW_Q_RXML,
      make_quartet(
          master_ml_options->distance_model, 
          master_ml_options->tmp_folder
      ) 
  );
  FCAL(
      GENERAL_ERROR,
      F_PROCESS_Q_IN_NEW_Q_RXML,
      process_quartet(master_ml_options->tmp_folder, name_map, u, res)
  );
  SYSCAL(
      GENERAL_ERROR, 
      ERR_RM, 
      "rm %s/%s %s/%s %s/RAxML_*", 
      master_ml_options->tmp_folder,
      STOCK_QUARTET_NAME, 
      master_ml_options->tmp_folder,
      STOCK_MSA_NAME, 
      master_ml_options->tmp_folder
  ); 

  return 0;
} 

int ml_quartet(
    char ** name_map, 
    int * u, 
    int * res, 
    ml_options * master_ml_options, 
    double * M)
{
  FCAL(
      GENERAL_ERROR,
      F_MATCH_PRINT_IN_ML_Q,
      match_and_print_msa(master_ml_options->tmp_folder, master_ml_options->input_alignment, name_map, u) 
  ); 

  FCAL(
      GENERAL_ERROR,
      F_DO_Q_LL_IN_ML_Q,
      do_quartet_ll(name_map, u, master_ml_options->distance_model, master_ml_options->tmp_folder, res, M)
  );

  SYSCAL(
      GENERAL_ERROR, 
      ERR_RM, 
      "rm %s/%s %s/%s %s/RAxML_*", 
      master_ml_options->tmp_folder,
      STOCK_QUARTET_NAME, 
      master_ml_options->tmp_folder, 
      STOCK_MSA_NAME,
      master_ml_options->tmp_folder
  );
  return 0;
}

int match_and_print_msa(char * tmp, char * aln, char ** name_map, int * u){
  FILE * f, * p;
  int i;
  int print_mode = 0;

  char buf[GENERAL_BUFFER_SIZE];
  char buf2[GENERAL_BUFFER_SIZE];
  sprintf(buf2, "%s/%s", tmp, STOCK_MSA_NAME);

  f = fopen(buf2, "w");
  p = SAFE_FOPEN_RD(aln);

  while(fscanf(p, "%s", buf) >= 0){
    for(i = 0; i < 4; i++){
      if(STR_EQ(&buf[1], name_map[u[i]]))
        print_mode = 1;
    }
    if(print_mode){
      if(buf[0] == '>'){
        print_mode = 0;
        for(i = 0; i < 4; i++){
          if(STR_EQ(&buf[1], name_map[u[i]]))
            print_mode = 1;
        }
        if(print_mode) fprintf(f, "%s\n", buf);
      } else 
        fprintf(f, "%s\n", buf);
    }
  }

  fclose(f);
  fclose(p);

  return 0;
}

int make_quartet(DIST_MOD distance_model, char * tmp_folder){
  char buf[GENERAL_BUFFER_SIZE];
  char msa_path[GENERAL_BUFFER_SIZE];
  sprintf(msa_path, "%s/%s", tmp_folder, STOCK_MSA_NAME);

  char quartet_path[GENERAL_BUFFER_SIZE];
  sprintf(quartet_path, "%s/%s", tmp_folder, STOCK_QUARTET_NAME);
// 

  // FILE * f = SAFE_FOPEN_RD(quartet_path);

  // if(f){
  //   fclose(f);
  //   ASSERT(GENERAL_ERROR, W_TMP_Q_EXIST, 0);
  // } 
  FCAL(
      GENERAL_ERROR,
      F_RXML_JOB_IN_MAKE_Q,
      raxml_job(
          distance_model, 
          tmp_folder,
          STOCK_QUARTET_NAME, 
          msa_path, 
          tmp_folder
      )
  );

  sprintf(buf, "%s/RAxML_bestTree.%s", tmp_folder, STOCK_QUARTET_NAME);

  FCAL(
      GENERAL_ERROR,
      F_RM_LBL_JOB_IN_MAKE_Q,
      rm_label_job(buf, quartet_path)
  );

  return 0;
}




int process_quartet(char * tmp_folder, char ** name_map, int * u, int * res){
  char sib[2][GENERAL_BUFFER_SIZE];

  char quartet_path[GENERAL_BUFFER_SIZE];
  sprintf(quartet_path, "%s/%s", tmp_folder, STOCK_QUARTET_NAME);

  FILE * f = SAFE_FOPEN_RD(quartet_path);
  int counter = 0, yes = 0;
  int idx[2], i;
  char cur_char;

  while(fscanf(f, "%c", &cur_char) >= 0){
    if(cur_char == '(') {
      yes = 1;
      counter = 0;
      continue;
    }
    if(yes == 1){
      if(cur_char == ',') {
        sib[0][counter] = 0;
        yes = 2;
        counter = 0;
      } else {
        sib[0][counter++] = cur_char;
      }
    } else if(yes == 2){
      if(cur_char == ')'){
        sib[1][counter] = 0;
        break;
      } else {
        sib[1][counter++] = cur_char;
      }
    }
  }
  fclose(f);

  for(i = 0; i < 2; ++i)
    for(idx[i] = 0; idx[i] < QUAD; ++idx[i])
      if(STR_EQ(sib[i], name_map[u[idx[i]]]))
        break;
    
  if(idx[0] == 3)
    *res = idx[1];
  else if(idx[1] == 3)
    *res = idx[0];
  else
    *res = (0 + 1 + 2 + 3) - idx[0] - idx[1] - 3;

  return 0;
}


int do_quartet_ll(
    char ** name_map, 
    int * u, 
    DIST_MOD distance_model, 
    char * tmp_folder,
    int * res, 
    double * M)
{
  int i;
  double ll[3], m;
  char cq[GENERAL_BUFFER_SIZE], buf[GENERAL_BUFFER_SIZE];

  char msa_path[GENERAL_BUFFER_SIZE];
  sprintf(msa_path, "%s/%s", tmp_folder, STOCK_MSA_NAME);

  char quartet_path[GENERAL_BUFFER_SIZE];
  sprintf(quartet_path, "%s/%s", tmp_folder, STOCK_QUARTET_NAME);
  FILE * f = fopen(quartet_path, "r"); 


  if(f){
    fclose(f);
    ASSERT(GENERAL_ERROR, W_TMP_Q_EXIST, 0);
  }

  STR_CLR(cq);
  for(i = 0; i < 3; ++i){
    sprintf(
        buf,
        "((%s,%s),(%s,%s))\n", 
        name_map[u[i]], 
        name_map[u[3]], 
        name_map[u[(i + 1) % 3]], 
        name_map[u[(i + 2) % 3]]
    );
    strcat(cq, buf);
  }

  FCAL(
      GENERAL_ERROR,
      F_GET_LL_IN_DO_Q_LL,
      get_ll_from_raxml(
          distance_model,
          tmp_folder,
          STOCK_QUARTET_NAME,
          msa_path,
          tmp_folder, 
          cq, 
          ll
      )
  );

  m = -1e9;
  for(i = 0; i < 3; ++i)
    if(ll[i] - m > -EPS){
      m = ll[i]; 
      *res = i;
      *M = 1.0 / 
          (1.0 + exp(1ll * (-ABS(ll[i] - ll[(i + 1) % 3]))) +
          exp(1ll * (-ABS(ll[i] - ll[(i + 2) % 3]))));
    }

  if(*M < 0.001) *M = 0.001;
  *M = 1.0/(*M);

  return 0;
}
