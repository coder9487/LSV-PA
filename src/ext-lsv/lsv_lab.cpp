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
  sat_solver_add_xor(pSat, 10, 20, 30, 0);

  lit Lits[1] = {Abc_Var2Lit(30, 0)};
  sat_solver_addclause(pSat, Lits, Lits + 1);


  int addClauseState = 1;
  int satState = 1;
  for (int j = 0; j <= 3 && satState == 1 && addClauseState == 1; j++)
  {
    satState = sat_solver_solve_internal(pSat);
    vector<int> vClause;
    printf("sat_solver_nvars:%d\n",sat_solver_nvars(pSat));
    for (int i = 0; i <= sat_solver_nvars(pSat) -1; i++)
    {
      vClause.push_back(sat_solver_var_value(pSat, i));
      printf("%d--->%d \n",i, sat_solver_var_value(pSat, i));
    }

    printf("\n");

    lit vLit[3] = {
      Abc_Var2Lit(1, vClause[1] ^ 0),
      Abc_Var2Lit(2, vClause[2] ^ 0),
      Abc_Var2Lit(3, vClause[3] ^ 0)};
    addClauseState = sat_solver_addclause(pSat, vLit, vLit + 3);

    printf("addClauseState: %d\n", addClauseState);
    printf("satState: %d\n", satState);
  }
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