#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "vector"
#include "map"
#include "math.h"




#include "booleanMatching.h"
#include "simulator.h"



extern "C"
{
  Abc_Ntk_t *Io_ReadBlifAsAig(char *pFileName, int fCheck);
}

static int Lsv_CommandFastBooleanMatching(Abc_Frame_t* pAbc, int argc, char** argv);






void init_fast_boolean_matching(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "LSV", "lsv_fbm", Lsv_CommandFastBooleanMatching, 0);
}

void destroy_fast_boolean_matching(Abc_Frame_t* pAbc) {}

Abc_FrameInitializer_t frame_initializer_fast_boolean_matching = {init_fast_boolean_matching, destroy_fast_boolean_matching};

struct PackageRegistrationManager {
  PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer_fast_boolean_matching); }
} lsvPackageRegistrationManager_fast_boolean_matching;

void Lsv_NtkFastBooleanMatching(Abc_Ntk_t* pNtk,int testCase) {

Abc_Ntk_t ** pArrNtk = ABC_ALLOC(Abc_Ntk_t *, 2);
Abc_Ntk_t * pMiterNtk;




char filePath[2][128] = {};
sprintf(filePath[0], "release/case%02d/circuit_1.blif", testCase);
sprintf(filePath[1], "release/case%02d/circuit_1.blif", testCase);

pArrNtk[0] = Io_ReadBlifAsAig(filePath[0], 1);
pArrNtk[1] = Io_ReadBlifAsAig(filePath[1], 1);


pArrNtk[0] = pNtk;


int   numPi = Abc_NtkPiNum(pArrNtk[0]);

for (size_t i = 0; i < pow(2,numPi); i++)
{
  u_int64_t result = Simulation_P(pArrNtk[0],i);
  printf("Output %d\n",result);

  Abc_Obj_t * pPoIteObj;
  int nItePo = 0;
  std::vector<Abc_Obj_t *> * partialSet = new std::vector<Abc_Obj_t *>;
  Abc_NtkForEachPo(pArrNtk[0],pPoIteObj,nItePo){
    findPartialSet_P(pPoIteObj,partialSet);
  }
  printObjValInNtk(pArrNtk[0]);
  printf("Partial Set: ");
  for (auto obj : *partialSet) {
    printf("%s ", Abc_ObjName(obj));
  }
  printf("\n");
  delete partialSet;
}



}

int Lsv_CommandFastBooleanMatching(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int argv_num;
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }


  if (argc <= 1)
  {
    printf("Please type number of case 0 ~ 10 \n");
    return 0;
  }
  argv_num = atoi(argv[1]);
  Lsv_NtkFastBooleanMatching(pNtk,argv_num);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_fbm [-h]\n");
  Abc_Print(-2, "\t        prints the nodes in the network\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}