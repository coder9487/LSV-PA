#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"

#include <queue>
#include <list>
#include <unordered_set>
#include <algorithm>

#define DEBUG_MESG_ENABLE 0
#define DUMP_ANSWER 1




using namespace std;

static int Lsv_CommandLab(Abc_Frame_t *pAbc, int argc, char **argv);

void init_lsvLab(Abc_Frame_t *pAbc)
{
  Cmd_CommandAdd(pAbc, "LSV", "lsv_lab", Lsv_CommandLab, 0);
}

void destroy_lsvLab(Abc_Frame_t *pAbc) {}

Abc_FrameInitializer_t frame_initializer_lsvLab = {init_lsvLab, destroy_lsvLab};

struct PackageRegistrationManager
{
  PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer_lsvLab); }
} lsvPackageRegistrationManager_lsvLab;




void Lsv_NtkLab(Abc_Ntk_t *pNtk)
{
  sat_solver *pSat = sat_solver_new();
  pSat->verbosity = 2;
  pSat->fPrintClause = 1;
  sat_solver_add_xor(pSat, 1, 2, 3, 0);

  lit Lits[1] = {1};
  sat_solver_addclause(pSat, Lits, Lits + 1);

for (int j = 0;j <= 3;j++){ 
   sat_solver_solve_internal(pSat);
  vector<bool> vClause;
  for(int i = 1;i <= 3;i++){
    vClause.push_back(sat_solver_get_var_value(pSat,i ));
    
    }
  printf("SAT: %d %d %d\n",vClause[0],vClause[1],vClause[2]);

  lit vLit[3] = {toLitCond(1,!vClause[0]),toLitCond(2,!vClause[1]),toLitCond(3,!vClause[2])};
  sat_solver_addclause(pSat, vLit, vLit + 3);}



  
}

int Lsv_CommandLab(Abc_Frame_t *pAbc, int argc, char **argv)
{

  int numberCutset = 3;
  Abc_Ntk_t *pNtk = Abc_FrameReadNtk(pAbc);
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF)
  {
    switch (c)
    {
    case 'h':
      goto usage;
    default:
      goto usage;
    }
  }


  Lsv_NtkLab(pNtk);
  
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_lab [-h]\n");
  Abc_Print(-2, "\t        prints the nodes in the network\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}