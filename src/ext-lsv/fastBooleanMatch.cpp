#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "vector"
#include "map"
#include "math.h"

#define READ_CUSTIOM_FILE 0
#define MAX_STA_SOLVING_TIMES 99999

#include "booleanMatching.h"
//#include "printObj.h"
//#include "simulator.h"



extern "C"
{
  Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters);
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







//std::tuple<Cnf_Dat_t *, Lit2Obj* >  createMiterCnf(int nPrimaryOutputs,Lit2Obj *Lit2Obj_Miter)
void **createMiterCnf(int nPrimaryOutputs)

{

  Abc_Ntk_t *pNtkMiter = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_AIG, 1);
  std::vector<int> vMiterNodePiId;
  std::vector<int> vMiterNodePoId;

  Cnf_Dat_t *pCnfMiter; 

  
  
  
  pNtkMiter->pName = Extra_UtilStrsav("Miter");
  Vec_Ptr_t *vNodeXor = Vec_PtrAlloc(nPrimaryOutputs);
  vNodeXor->nSize = nPrimaryOutputs;
  
  for (int i = 0; i < nPrimaryOutputs; i++)
  {
    Vec_Ptr_t *vNodePi = Vec_PtrAlloc(2);
    vNodePi->nSize = 2;
    vNodePi->pArray[0] = (void **)Abc_NtkCreatePi(pNtkMiter);
    vNodePi->pArray[1] = (void **)Abc_NtkCreatePi(pNtkMiter);
    vMiterNodePiId.push_back(Abc_ObjId((Abc_Obj_t *)vNodePi->pArray[0]));
    vMiterNodePiId.push_back(Abc_ObjId((Abc_Obj_t *)vNodePi->pArray[1]));
    vNodeXor->pArray[i] = (void **)Abc_NtkCreateNodeExor(pNtkMiter, vNodePi);
  }
  Abc_Obj_t *pObjOr = Abc_NtkCreateNodeOr(pNtkMiter, vNodeXor);
  Abc_Obj_t *pObjPo = Abc_NtkCreatePo(pNtkMiter);

  Abc_ObjAddFanin(pObjPo, pObjOr);
  pNtkMiter = Abc_NtkStrash(pNtkMiter, 1, 0, 0);
  Aig_Man_t *pAigMan = Abc_NtkToDar(pNtkMiter, 0, 0);

  vMiterNodePoId.push_back(Abc_ObjId(Abc_ObjFanin0(pObjPo)));
  pCnfMiter = Cnf_Derive(pAigMan, 1);


  //* Connect POs in both pNtk1 and pNtk2 to Miter
 // LiftOffset = LiftSize(LiftOffset + Lit2Obj_ckt1->numPi * 2 * Lit2Obj_ckt2->numPi);
  //Cnf_DataLift(pCnfMiter, LiftOffset);
  
  Lit2Obj* Lit2Obj_Miter = collectLitid2Objid(pNtkMiter, pCnfMiter);
  
  void** arr = new void*[2];
  arr[0] = (void*)pCnfMiter;
  arr[1] = (void*)Lit2Obj_Miter;
return arr;
/*
  std::tuple<Cnf_Dat_t *, Lit2Obj* > miterTuple 
  = std::make_tuple(pCnfMiter, Lit2Obj_Miter);

  return miterTuple;
*/


}







void Lsv_NtkFastBooleanMatching(Abc_Ntk_t* pNtk,int testCase) {

Abc_Ntk_t ** pArrNtk = ABC_ALLOC(Abc_Ntk_t *, 2);
Abc_Ntk_t * pMiterNtk;




char filePath[2][128] = {};
sprintf(filePath[0], "release/case%02d/circuit_1.blif", testCase);
sprintf(filePath[1], "release/case%02d/circuit_2.blif", testCase);

pArrNtk[0] = Io_ReadBlifAsAig(filePath[0], 1);
pArrNtk[1] = Io_ReadBlifAsAig(filePath[1], 1);
#if READ_CUSTIOM_FILE
  pArrNtk[0] = pNtk;
  pArrNtk[0] = Abc_NtkStrash(pArrNtk[0], 1, 0, 1);
#endif

// RouteTable *pRouteTablePIs = new RouteTable();
//Lit2Obj *Lit2Obj_Miter ;

//std::tuple<Cnf_Dat_t *, Lit2Obj* > miterTuple = createMiterCnf(Abc_NtkPoNum(pArrNtk[0]), Lit2Obj_Miter);
//Cnf_Dat_t *pCnfMiter = std::get<0>(miterTuple);
//Lit2Obj_Miter = std::get<1>(miterTuple);
//Cnf_Dat_t * pCnfMiter = ;


//************************************************************ 
//*
//*                     Create Miter
//*
//************************************************************


void** arr = createMiterCnf(Abc_NtkPoNum(pArrNtk[0]));
Cnf_Dat_t   * pCnfMiter       =  (Cnf_Dat_t * )arr[0];
Lit2Obj     * Lit2Obj_Miter   = (Lit2Obj *)arr[1];
Cnf_DataPrint(pCnfMiter, 1);
printLit2Obj(Lit2Obj_Miter,"Lit2Obj_Miter");
int Miter_nCnf = pCnfMiter->nVars;

//************************************************************ 
//*
//*                     Create Ckt1 CNF and Ckt2 CNF
//*
//************************************************************



CircuitMan * pCircuit1 = new CircuitMan(pArrNtk[0]);
CircuitMan * pCircuit2 = new CircuitMan(pArrNtk[1]);

pCircuit1->toCnf(Miter_nCnf);
int ckt1_nCnf = pCircuit1->pCnf->nVars;
pCircuit2->toCnf(Miter_nCnf + ckt1_nCnf);
int ckt2_nCnf = pCircuit2->pCnf->nVars;




Cnf_DataPrint(pCircuit1->pCnf, 1);
Cnf_DataPrint(pCircuit2->pCnf, 1);


//* Prepare x index 
std::vector<int>* pCkt2_vecPoLit = &(pCircuit2->lit2obj->vec_poLitId);
std::vector<int> pCkt2_Pox2;
for (size_t i = 0; i < pCkt2_vecPoLit->size(); i++)
{
  pCkt2_Pox2.push_back(pCkt2_vecPoLit->at(i) );
  pCkt2_Pox2.push_back(-pCkt2_vecPoLit->at(i) );
}


//* 
std::vector<int> pMiter_vecFanin1;
for (size_t i = 1; i < Lit2Obj_Miter->vec_piLitId.size(); i += 2)
{
  pMiter_vecFanin1.push_back(Lit2Obj_Miter->vec_piLitId[i]);
} 


RouteTable *pPORouteTable = new RouteTable(pCkt2_Pox2,pMiter_vecFanin1,Miter_nCnf + ckt1_nCnf + ckt2_nCnf);

pPORouteTable->printRouteTable();




//************************************************************ 
//*
//*        Connect Ckt1 CNF Miter CNF of PO box
//*
//************************************************************


std::vector<int> pMiter_vecFanin0;
for (size_t i = 0; i < Lit2Obj_Miter->vec_piLitId.size(); i += 2)
{
  pMiter_vecFanin0.push_back(Lit2Obj_Miter->vec_piLitId[i]);
} 
std::vector<std::vector<int>>  vvCNFwire = cnfConnection(pCircuit1->lit2obj->vec_poLitId,pMiter_vecFanin0);



//************************************************************ 
//*
//*         Connect Ckt1 CNF Ckt2 CNF of PI box
//*
//************************************************************


std::vector<int>* pCkt1_vecPiLit = &(pCircuit1->lit2obj->vec_piLitId);
std::vector<int> pCkt1_Pix2;
for (size_t i = 0; i < pCkt1_vecPiLit->size(); i++)
{
  pCkt1_Pix2.push_back(pCkt1_vecPiLit->at(i) );
  pCkt1_Pix2.push_back(-pCkt1_vecPiLit->at(i) );
}

RouteTable *pPIRouteTable = new RouteTable(pCkt1_Pix2,pCircuit2->lit2obj->vec_piLitId, pPORouteTable->cnfLitMax);
 pPIRouteTable->printRouteTable();



//************************************************************ 
//*
//*         Sat Solver Initialization
//*
//************************************************************


sat_solver * pMatchingSat = sat_solver_new();


pMatchingSat->fPrintClause    = 1;
pMatchingSat->verbosity       = 2;



Cnf_DataWriteIntoSolverInt(pMatchingSat,pCnfMiter,1,0);
Cnf_DataWriteIntoSolverInt(pMatchingSat,pCircuit1->pCnf,1,0);
Cnf_DataWriteIntoSolverInt(pMatchingSat,pCircuit2->pCnf,1,0);





vvWriteToSat((pPORouteTable->vvRoutePathCNF),pMatchingSat,0);
vvWriteToSat((pPORouteTable->vvRouteTableRuleCNF),pMatchingSat,0);
vvWriteToSat((pPIRouteTable->vvRoutePathCNF),pMatchingSat,0);
vvWriteToSat((pPIRouteTable->vvRouteTableRuleCNF),pMatchingSat,0);

vvWriteToSat(vvCNFwire,pMatchingSat,0);

sat_solver_add_const(pMatchingSat,Lit2Obj_Miter->vec_poLitId[0],0);

printLit2Obj(Lit2Obj_Miter,"Lit2Obj_Miter");
Cnf_DataPrint(pCnfMiter, 1);

//************************************************************ 
//*
//*         Sat Solver Solving
//*
//************************************************************


u_int8_t SatStatus = sat_solver_solve_internal(pMatchingSat);
u_int8_t AddClauseStatus = 2;
std::vector<std::vector<int>> vvLearnClause;
std::vector<int> vvRoutePathFilter;
std::vector<int> vSatisfyRoutePath;


vvRoutePathFilter = pPORouteTable->getRouteTable1D();
std::vector<int> piRouteTable1D = pPIRouteTable->getRouteTable1D();
vvRoutePathFilter.insert(vvRoutePathFilter.end(), piRouteTable1D.begin(), piRouteTable1D.end());


std::vector<std::vector<int>> vvlearnclause;
size_t sat_cnt = 0;
for (;sat_cnt < MAX_STA_SOLVING_TIMES && SatStatus == 1 && AddClauseStatus == 2;sat_cnt++)
{
  AddClauseStatus = 0;
  vvlearnclause.clear();
  vSatisfyRoutePath.clear();
  std::vector<int> vCkt1SatVar = getSatAnswer(pMatchingSat,&pCircuit1->lit2obj->vec_piLitId);
  std::vector<int> vCkt2SatVar = getSatAnswer(pMatchingSat,&pCircuit2->lit2obj->vec_piLitId);
  vSatisfyRoutePath = getSatAnswer(pMatchingSat,&vvRoutePathFilter);

  u_int64_t ckt1Pattern =  pCircuit1->Cnf2Obj2Pattern(vCkt1SatVar);
  u_int64_t ckt2Pattern =  pCircuit2->Cnf2Obj2Pattern(vCkt2SatVar);
  
  pCircuit1->Simulation(ckt1Pattern);
  pCircuit1->PartialAssignment();
  pCircuit1->printvSuppObj();
  pCircuit1->printObjValInNtk();

  pCircuit2->Simulation(ckt2Pattern);
  pCircuit2->PartialAssignment();
  pCircuit2->printvSuppObj();
  pCircuit2->printObjValInNtk();


  vvlearnclause = (LearnClause(pCircuit1,pCircuit2,pPORouteTable,pPIRouteTable));
  for (size_t i = 0; i < vvlearnclause.size(); i++)
  {
    vvLearnClause.push_back(vvlearnclause[i]);
  }
  

  AddClauseStatus += vvWriteToSat(vvlearnclause,pMatchingSat,0);
  AddClauseStatus += vWriteToSat(vSatisfyRoutePath,pMatchingSat,1);
}

return;

//*
/*
pCircuit1->Simulation(5);
pCircuit1->PartialAssignment();
pCircuit1->printvSuppObj();

pCircuit2->Simulation(3);
pCircuit2->PartialAssignment();
pCircuit2->printvSuppObj();
*/


/*
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
*/


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
#if READ_CUSTIOM_FILE
  if(pNtk == NULL){
    printf("NULL Network\n");
    return 0;
  }
#endif
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