#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "sat/cnf/cnf.h"


#include <map>
#include <vector>
#include <list>
#include <string.h>
#include <algorithm>
#include <ctype.h>
#include <string>
#include "simulator.h"


#define DUMP_BUS_GROUP_INFO 0
#define READ_BLIFFILE 1

using namespace std;

//# define READ_PIPO_GROUP

#define LiftSize(x) ((x + 1)*1)

extern "C"
{
  void Abc_NtkShow(Abc_Ntk_t *pNtk0, int fGateNames, int fSeq, int fUseReverse, int fKeepDot, int fAigIds);
  Abc_Ntk_t *Io_ReadBlifAsAig(char *pFileName, int fCheck);
  Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters);
  Abc_Ntk_t *Abc_NtkDouble(Abc_Ntk_t *pNtk);
  void Abc_NtkMiterPrepare( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, Abc_Ntk_t * pNtkMiter, int fComb, int nPartSize, int fMulti );
  
}



static int Lsv_Command_BooleanMatching(Abc_Frame_t *pAbc, int argc, char **argv);

void init_BooleanMatching(Abc_Frame_t *pAbc)
{
  Cmd_CommandAdd(pAbc, "LSV", "lsv_match", Lsv_Command_BooleanMatching, 0);
}

void destroy_BooleanMatching(Abc_Frame_t *pAbc) {}

Abc_FrameInitializer_t frame_initializer_BooleanMatching = {init_BooleanMatching, destroy_BooleanMatching};

struct PackageRegistrationManager
{
  PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer_BooleanMatching); }
} lsvPackageRegistrationManager_BooleanMatching;



typedef struct LIT2OBJ
{
  int numPi;
  int numPo;
  vector<int> vec_piObjId;
  vector<int> vec_piLitId;
  vector<int> vec_poObjId;
  vector<int> vec_poLitId;
} Lit2Obj;





void readBusGroup(char *filePath,  vector<vector<string >> &busGroup1, vector<vector<string >> &busGroup2)
{


  char line[1024];
  FILE *file = fopen(filePath, "r");
  if (file == NULL)
  {
    perror("Error opening file");
    return;
  }
  

  int lineCnt = 0;
  int circuitCnt = 0;
  int lineOffset = 9999;
  int groupCnt = 0;
  int elementCnt = 0;
  

  while (fgets(line, sizeof(line), file))
  {
    if (lineCnt == 0 || lineCnt == lineOffset + 1 );
    else if (lineCnt == 1 || lineCnt == lineOffset + 1 + 1)
    {
      //printf("Line: %s\n", line);
      //printf("busNum: %d\n", busNum);
      lineOffset = atoi(line) + 1;
      groupCnt = 0;
      circuitCnt++;      
    }
    else
    {
      elementCnt = 0;
      char *token = strtok(line, " \n");
      
      {
        //printf("\tGroup ele cnt %d\n", atoi(token));
        if(circuitCnt == 1)
          busGroup1.push_back(vector<string>());
        else
          busGroup2.push_back(vector<string>());
      }
      elementCnt++;

      while (token != NULL)
      {
        //printf("\t\tToken: %s\n", token);
        if(elementCnt != 1)
        {
          if(circuitCnt == 1)
            busGroup1[groupCnt].push_back (token);
          else
            busGroup2[groupCnt].push_back (token);
        }


        token = strtok(NULL, " \n");
        elementCnt++;
      }
      groupCnt++;
    }
    lineCnt++;

  }

  fclose(file);



}
void dumpBusGroup(vector<vector<string>> busGroup1)
{
  printf("Bus Group:\n");
  for (size_t i = 0; i < busGroup1.size(); ++i)
  {
    printf("Group %d:\n", i);
    for (size_t j = 0; j < busGroup1[i].size(); ++j)
    {
      printf("\t%s\n", busGroup1[i][j].c_str());
    }
  }
}
void dumpBusGroup(vector<vector<int>> busGroup1)
{
  printf("Bus Group:\n");
  for (size_t i = 0; i < busGroup1.size(); ++i)
  {
    printf("Group %d:\n", i);
    for (size_t j = 0; j < busGroup1[i].size(); ++j)
    {
      printf("\t%d\n", busGroup1[i][j]);
    }
  }

}
void extracBusGroupVector(  vector<vector<string>> &InputStringVector,map<int,string> &OutputVector)
{
  int k = 0;
  for (size_t i = 0; i < InputStringVector.size(); ++i)
  {
    for (size_t j = 0; j < InputStringVector[i].size(); ++j)
    {
      OutputVector[k] = InputStringVector[i][j];
      k++;
    }
     
  }
}
void findPiPoNodeId(Abc_Ntk_t *pNtk, map<int,string> &lookupTable)
{
  Abc_Obj_t *pObj;
  int i;

  Abc_NtkForEachPi(pNtk, pObj, i)
  {
    char* nodeName = Abc_ObjName(pObj);
    int nodeId = Abc_ObjId(pObj);
    lookupTable[nodeId] = nodeName;
  }
  Abc_NtkForEachPo(pNtk, pObj, i)
  {
    char* nodeName = Abc_ObjName(pObj);
    int nodeId = Abc_ObjId(pObj);
    lookupTable[nodeId] = nodeName;
  }
}
void matchBusIdName(map<int,string> loookupTable, vector<vector<int >> &busGroupId, vector<vector<string >> &busGroupName)
{
  for (size_t i = 0; i < busGroupName.size(); ++i)
  {
    busGroupId.push_back(vector<int>());
    for (size_t j = 0; j < busGroupName[i].size(); ++j)
    {
      for (const auto &pair : loookupTable)
      {
        if (pair.second == busGroupName[i][j])
        {
          busGroupId[i].push_back(pair.first);
          break;
        }
      }
    }
  }
}
void renameObj(Abc_Ntk_t *pNtk,char * circuitName)
{
  Abc_Obj_t *pObj;
  int i;
  Abc_NtkForEachObj(pNtk, pObj, i)
  {

    char* nodeName = Abc_ObjName(pObj);
    char newName [64];
    sprintf(newName, "%s_%s",circuitName, nodeName);
    
    printf("Old obj Name: %s ID: %d \t",Abc_ObjName(pObj),Abc_ObjId(pObj));
    Abc_ObjAssignName(pObj, newName, NULL);
    printf("New obj Name: %s ID: %d\n",Abc_ObjName(pObj),Abc_ObjId(pObj));

    
  }

}
void PrintObjName(Abc_Ntk_t *pNtk)
{
  Abc_Obj_t *pObj;
  int i;
  Abc_NtkForEachObj(pNtk, pObj, i)
  {
    printf("Obj name: %s \t id:%d\n",Abc_ObjName(pObj),Abc_ObjId(pObj));
  }
}




//* Boolean matching */
typedef struct RouteTable_t
{
  int **ppTable;
  int *pRow;
  int *pCol;
  int nRow;
  int nCol;
} RouteTable;

void printRouteTable(RouteTable * s, int row, int col) 
{
  for (int i = 0; i < row; i++) {
    for (int j = 0; j < col; j++) {
      printf("%d ", s->ppTable[i][j]);
    }
    printf("\n");
  }
}


int vvClauseaddClause(vector<vector<int>> * vvClause, vector<int> vClause)
{
  vvClause->push_back(vClause);
  return 0;
}

void vvPrint(vector<vector<int>> * vvClause)
{
  for (size_t i = 0; i < vvClause->size(); ++i)
  {
    for (size_t j = 0; j < vvClause->at(i).size(); ++j)
    {
      printf("%d ", vvClause->at(i).at(j));
    }
    printf("\n");
  }
  return;
}

void createRouteTableRule(RouteTable_t * pTable,vector<vector<int>> * vvConstraint1)
{
  

  for (int i = 0; i < pTable->nCol;i += 2 )
  {
    vector<int> vvSubArray;
    for (int j = 0; j < pTable->nRow;j++)
    {
      vvSubArray.push_back(pTable->ppTable[j][i]);
      vvSubArray.push_back(pTable->ppTable[j][i+1]);
    }
    vvConstraint1->push_back(vvSubArray);
  }
  for (int i = 0; i < pTable->nRow;i++)
  {
    vector<int> vvSubArray;
    for (int j = 0; j < pTable->nCol;j++)
    {
      vvSubArray.push_back(pTable->ppTable[i][j]);
    }
    vvConstraint1->push_back(vvSubArray);
  }

}

void routeTable2Vec(RouteTable_t * pTable,vector<int> * vvConstraint1)
{
  for (int i = 0; i < pTable->nRow; ++i)
  {
    vector<int> vLine;
    for (int j = 0; j < pTable->nCol; ++j)
    {
      vvConstraint1->push_back(pTable->ppTable[i][j]);
    }
    
  }
}

void converRouteTable2Clauses(vector<vector<int>> * vvConstraint1,vector<vector<int>>* vvClause){
  for (size_t i = 0; i < vvConstraint1->size(); ++i)
  {
    vector<int> vLine = vvConstraint1->at(i);
    vvClauseaddClause(vvClause,vLine);
    for (size_t base_index = 0; base_index < vLine.size(); base_index++)
    {
      
      for(size_t move_index = base_index+1; move_index < vLine.size(); move_index++)
      {
        vector<int> vClause;
        vClause.push_back(-vLine[base_index]);
        vClause.push_back(-vLine[move_index]);
        vvClauseaddClause(vvClause,vClause);
        //for (int val : vClause) printf("%d ", val);printf("\n");
      }
    }
    //printf("\n");
  }

}

vector<vector<int>> * createRouteTable(RouteTable_t * pTable, int litStartFrom, int numX,int numY,int *xIndex,int *yIndex)//,
{
  vector<vector<int>> * vvClause = new vector<vector<int>>;
  //? vector<int> *vectorName = new vector<int>(366, vector<int>(4));
  pTable->nRow = numY;
  pTable->nCol = 2*numX;
  //_createRouteTable(pTable->ppTable,litStartFrom, pTable->nRow, pTable->nCol);
  pTable->ppTable = new int*[pTable->nRow];
  for (int y_ite = 0; y_ite < pTable->nRow; ++y_ite) {
    pTable->ppTable[y_ite] = new int[pTable->nCol];
    for (int x_ite = 0; x_ite < pTable->nCol; ++x_ite) {
      pTable->ppTable[y_ite][x_ite] = y_ite * pTable->nCol + x_ite + litStartFrom; 

      
      
      //printf("X[x_ite]: %d Y[y_ite]: %d\n",xIndex[x_ite],yIndex[y_ite]);
      //* When xIndex[x_ite] is negative, clause is -|x| -|y| which -|x| => -(-x) =>x
      //* and y remains the same
        vector<int> vClause ;
        vClause = {-pTable->ppTable[y_ite][x_ite],-xIndex[x_ite],yIndex[y_ite]};
        //for (int val : vClause) printf("%d ", val);printf("\n");
        vvClause->push_back(vClause);

        vClause = {-pTable->ppTable[y_ite][x_ite],xIndex[x_ite],-yIndex[y_ite]};
        //for (int val : vClause) printf("%d ", val);printf("\n");
        vvClause->push_back(vClause);



    }
  }
  return vvClause;
}

Lit2Obj * collectLitid2Objid(Abc_Ntk_t * pNtk,Cnf_Dat_t * pCnf)
{
  Lit2Obj *Lit2Obj_1 = new Lit2Obj;
  Lit2Obj_1->numPi = Abc_NtkPiNum(pNtk);
  Lit2Obj_1->numPo = Abc_NtkPoNum(pNtk);


  Lit2Obj_1->vec_piLitId.resize(Lit2Obj_1->numPi);
  Lit2Obj_1->vec_piObjId.resize(Lit2Obj_1->numPi);
  Lit2Obj_1->vec_poLitId.resize(Lit2Obj_1->numPo);
  Lit2Obj_1->vec_poObjId.resize(Lit2Obj_1->numPo);


  
  Abc_Obj_t *pObj;
  int i;
  Abc_NtkForEachPi(pNtk, pObj, i)
  {
    Lit2Obj_1->vec_piLitId[i] = pCnf->pVarNums[Abc_ObjId(pObj)];
    Lit2Obj_1->vec_piObjId[i] = Abc_ObjId(pObj);
  }
  Abc_NtkForEachPo(pNtk, pObj, i)
  {
    
    //printf("@PO: %d \t",Abc_ObjId(pObj));
    Abc_Obj_t *pObjTmp = pObj;  
    int  litId = pCnf->pVarNums[Abc_ObjId(pObjTmp)];
    
    while(litId == -1)
    {
      pObjTmp = Abc_ObjFanin0(pObjTmp);
      
      litId = pCnf->pVarNums[Abc_ObjId(pObjTmp)];
      //printf("Get Fanin ID instead: %d\n",Abc_ObjId(pObjTmp));
    }

    Lit2Obj_1->vec_poLitId[i] = litId;
    Lit2Obj_1->vec_poObjId[i] = Abc_ObjId(pObj);
  }
  return Lit2Obj_1;
}

Lit2Obj * collectLitid2Objid(vector<int> *vPis,vector<int> *vPos,Cnf_Dat_t * pCnf)
{
  Lit2Obj *Lit2Obj_1 = new Lit2Obj;
  Lit2Obj_1->numPi = vPis->size();
  Lit2Obj_1->numPo = vPos->size();


  Lit2Obj_1->vec_piLitId.resize(Lit2Obj_1->numPi);
  Lit2Obj_1->vec_piObjId.resize(Lit2Obj_1->numPi);
  Lit2Obj_1->vec_poLitId.resize(Lit2Obj_1->numPo);
  Lit2Obj_1->vec_poObjId.resize(Lit2Obj_1->numPo);


  
  Abc_Obj_t *pObj;
  int i;
  for (size_t i = 0; i < vPis->size(); ++i)
  {
    Lit2Obj_1->vec_piLitId[i] = pCnf->pVarNums[vPis->at(i)];
    Lit2Obj_1->vec_piObjId[i] = vPis->at(i);
  }
  for (size_t i = 0; i < vPos->size(); ++i)
  {
    
    //printf("@PO: %d \t",Abc_ObjId(pObj));
    int  litId = pCnf->pVarNums[vPos->at(i)];
    while(litId == -1)
    {
      //Abc_Obj_t *pPoObjFanin = Abc_ObjFanin0(pObj);
      
      litId = pCnf->pVarNums[vPos->at(i)];
      //printf("Get Fanin ID instead: %d\n",Abc_ObjId(pPoObjFanin));
    }

    Lit2Obj_1->vec_poLitId[i] = litId;
    Lit2Obj_1->vec_poObjId[i] = vPos->at(i);
  }

  return Lit2Obj_1;
}

void printLit2Obj(Lit2Obj *Lit2Obj_1)
{
  printf("PI Lit ID:\n");
  for (size_t i = 0; i < Lit2Obj_1->vec_piLitId.size(); ++i)
  {
    printf("%d ", Lit2Obj_1->vec_piLitId[i]);
  }
  printf("\n");
  printf("PI Obj ID:\n");
  for (size_t i = 0; i < Lit2Obj_1->vec_piObjId.size(); ++i)
  {
    printf("%d ", Lit2Obj_1->vec_piObjId[i]);
  }
  printf("\n");
  printf("PO Lit ID:\n");
  for (size_t i = 0; i < Lit2Obj_1->vec_poLitId.size(); ++i)
  {
    printf("%d ", Lit2Obj_1->vec_poLitId[i]);
  }
  printf("\n");
  printf("PO Obj ID:\n");
  for (size_t i = 0; i < Lit2Obj_1->vec_poObjId.size(); ++i)
  {
    printf("%d ", Lit2Obj_1->vec_poObjId[i]);
  }
  printf("\n");
}
int vvWriteToSat(vector<vector<int>> * vvClause, sat_solver * sat)
{
  for (size_t i = 0; i < vvClause->size(); ++i)
  {
    vector<int> vClause ;
    for (size_t j = 0; j < vvClause->at(i).size(); ++j)
    {
      int lit = abs((vvClause->at(i).at(j)))*2;
      int sign = vvClause->at(i).at(j) < 0 ? 1 : 0;
      vClause.push_back(lit + sign);
    }
    if (!sat_solver_addclause(sat, vClause.data(), vClause.data()+vClause.size()))
    {
      
      return 0;
    }
  }
  return 1;
}
int vWriteToSat(vector<int> vClause, sat_solver * sat,int fBlockAns)
{
  vector<int> vClause_;
  for (size_t i = 0; i < vClause.size(); ++i)
  {
    int lit = abs((vClause.at(i)));
    int sign = vClause.at(i) < 0 ? 1 : 0;
    vClause_.push_back(toLitCond(lit,sign)^ fBlockAns);
  }
  if (sat_solver_addclause(sat, vClause_.data(), vClause_.data()+vClause_.size()) != 1)
  {
    return 0;
  }
  return 1;
}


vector<int>  getSatAnswer(sat_solver * sat,vector<int> *filter)
{
  vector<int> vecResult;// = new vector<int>;
  for (size_t i = 0; i < filter->size(); ++i)
  {
    //printf("%d ",filter->at(i),sat_solver_var_value(sat, filter->at(i)));
    int lit = filter->at(i);
    int sign = sat_solver_var_value(sat, lit);
    lit =  sign ? -lit : lit;
    vecResult.push_back(lit);
  }
  return vecResult;
}













void Lsv_Ntk_BooleanMatching(int testCase)
{

  //* Read Input file */

  char groupFilePath[64] = {};
  sprintf(groupFilePath, "release/case%02d/input", testCase);
  vector<vector<string >> busGroup1Name;
  vector<vector<string >> busGroup2Name;
#ifdef READ_PIPO_GROUP
  map<int, string> busGroup1Map;
  map<int, string> busGroup2Map;

  readBusGroup(groupFilePath,busGroup1Name,busGroup2Name);

  extracBusGroupVector(busGroup1Name,busGroup1Map);
  extracBusGroupVector(busGroup2Name,busGroup2Map);
  #endif
  //* Read Circuit file */

  char filePath[2][128] = {};
  Abc_Ntk_t **Abc_Ntk_arr = ABC_ALLOC(Abc_Ntk_t *, 2);


#if READ_BLIFFILE
  sprintf(filePath[0], "release/case%02d/circuit_1.blif", testCase);
  sprintf(filePath[1], "release/case%02d/circuit_2.blif", testCase);
  Abc_Ntk_arr[0] = Io_ReadBlifAsAig(filePath[0], 1);
  Abc_Ntk_arr[1] = Io_ReadBlifAsAig(filePath[1], 1);
#else
  sprintf(filePath[0], "release/case%02d/circuit_1.aig", testCase);
  sprintf(filePath[1], "release/case%02d/circuit_2.aig", testCase);
  Abc_Ntk_arr[0] = Io_ReadAiger(filePath[0], 1);
  Abc_Ntk_arr[1] = Io_ReadAiger(filePath[1], 1);
#endif



  if (!Abc_NtkIsStrash(Abc_Ntk_arr[0]))
    Abc_NtkStrash(Abc_Ntk_arr[0], 1, 0, 1);
  if (!Abc_NtkIsStrash(Abc_Ntk_arr[1]))
    Abc_NtkStrash(Abc_Ntk_arr[1], 1, 0, 1);
  

  Abc_Ntk_arr[0]->pName = Extra_UtilStrsav("Circuit1");
  Abc_Ntk_arr[1]->pName = Extra_UtilStrsav("Circuit2");


  //* Find PI Node ID */
#ifdef READ_PIPO_GROUP
  map<int,string> lookupTable1;
  map<int,string> lookupTable2;

  findPiPoNodeId(Abc_Ntk_arr[0],lookupTable1);
  findPiPoNodeId(Abc_Ntk_arr[1],lookupTable2);




  //* Match Bus ID and Name */
  vector<vector<int >> busGroup1Id;
  vector<vector<int >> busGroup2Id;

  matchBusIdName(lookupTable1,busGroup1Id,busGroup1Name);
  matchBusIdName(lookupTable2,busGroup2Id,busGroup2Name);


  busGroup1Name.clear();
  busGroup2Name.clear();
  busGroup1Map.clear();
  busGroup2Map.clear();
  lookupTable1.clear();
  lookupTable2.clear();


  //dumpBusGroup(busGroup1Id);
  //dumpBusGroup(busGroup2Id);

  // Measure time of the following process
  
  renameObj(Abc_Ntk_arr[1],"ckt2");
  renameObj(Abc_Ntk_arr[0],"ckt1");

  


  PrintObjName(Abc_Ntk_arr[0]);
  PrintObjName(Abc_Ntk_arr[1]);


#endif

  Abc_Ntk_t *pNtkCkt1 = Abc_NtkDup(Abc_Ntk_arr[0]);
  Aig_Man_t *pManCkt1 = Abc_NtkToDar(pNtkCkt1, 0, 0);
  Cnf_Dat_t *pCnfCkt1 = Cnf_Derive(pManCkt1, Abc_NtkPoNum(pNtkCkt1));
  int maxLitCkt1 = pCnfCkt1->nVars;
  Lit2Obj *Lit2Obj_ckt1 = collectLitid2Objid(pNtkCkt1,pCnfCkt1);
  

  Abc_Ntk_t *pNtkCkt2 = Abc_NtkDup(Abc_Ntk_arr[1]);
  Aig_Man_t *pManCkt2 = Abc_NtkToDar(pNtkCkt2, 0, 0);
  Cnf_Dat_t *pCnfCkt2 = Cnf_Derive(pManCkt2, Abc_NtkPoNum(pNtkCkt2));
  int maxLitCkt2 = pCnfCkt2->nVars;
  int LiftOffset = LiftSize(maxLitCkt1);
  Cnf_DataLift(pCnfCkt2, LiftOffset);
  Lit2Obj *Lit2Obj_ckt2 = collectLitid2Objid(pNtkCkt2,pCnfCkt2);

  
  //* Create xIndex and yIndex of PIs */
  int *ckt1PiArr = new int [Lit2Obj_ckt1->numPi * 2]; 
  int *ckt2PiArr = new int [Lit2Obj_ckt2->numPi];

  for (int i = 0; i < Lit2Obj_ckt1->numPi; i++){
    ckt1PiArr[2*i] = Lit2Obj_ckt1->vec_piLitId[i];
    ckt1PiArr[2*i+1] = -Lit2Obj_ckt1->vec_piLitId[i];
  }
    
  for (int i = 0; i < Lit2Obj_ckt2->numPi; i++)
    ckt2PiArr[i] = Lit2Obj_ckt2->vec_piLitId[i];




  // * Create PI permutation and negation table */
  LiftOffset = LiftSize(LiftOffset+maxLitCkt2);
  RouteTable *pPiTable = new RouteTable;
  vector<vector<int>> *vvClausePi = createRouteTable(
    pPiTable,
    LiftOffset,
    Lit2Obj_ckt1->numPi,
    Lit2Obj_ckt2->numPi,
    ckt1PiArr,
    ckt2PiArr);

  printRouteTable(pPiTable, pPiTable->nRow, pPiTable->nCol);
  
  vector<vector<int>> vvPiTableRule;
  vector<vector<int>> vvClausePiRule;
  createRouteTableRule(pPiTable,&vvPiTableRule);
  converRouteTable2Clauses(&vvPiTableRule,&vvClausePiRule);
  vvPiTableRule.clear();




  //* Create xIndex and yIndex of pos */
  int *ckt1PoArr = new int [Lit2Obj_ckt1->numPo * 2]; 
  int *ckt2PoArr = new int [Lit2Obj_ckt2->numPo];

  for (int i = 0; i < Lit2Obj_ckt1->numPo; i++){
    ckt1PoArr[2*i] = Lit2Obj_ckt1->vec_poLitId[i];
    ckt1PoArr[2*i+1] = -Lit2Obj_ckt1->vec_poLitId[i];
  }
    
  for (int i = 0; i < Lit2Obj_ckt2->numPo; i++)
    ckt2PoArr[i] = Lit2Obj_ckt2->vec_poLitId[i];




  // * Create PO permutation and negation table */
  LiftOffset = LiftSize(LiftOffset+Lit2Obj_ckt1->numPi*2*Lit2Obj_ckt2->numPi);
  RouteTable *pPoTable = new RouteTable;
  vector<vector<int>> *vvClausePo = createRouteTable(
    pPoTable,
    LiftOffset,
    Lit2Obj_ckt1->numPo,
    Lit2Obj_ckt2->numPo,
    ckt1PoArr,
    ckt2PoArr);
  
  
  //vvPrint(vvClausePo);
  //printRouteTable(pPoTable, pPoTable->nRow, pPoTable->nCol);

  vector<vector<int>> vvPoTableRule;
  vector<vector<int>> vvClausePoRule;
  createRouteTableRule(pPoTable,&vvPoTableRule);
  converRouteTable2Clauses(&vvPoTableRule,&vvClausePoRule);
  vvPoTableRule.clear();
  


  Abc_Ntk_t *pNtkMiter = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_AIG, 1);
  vector<int> vMiterNodePiId;
  vector<int> vMiterNodePoId;



  Cnf_Dat_t *pCnfMiter ;//= createMiterOrCnf(Abc_NtkPiNum(pNtkCkt1),pNtkMiter,&vMiterNodePiId,&vMiterNodePoId);



  //* Create Miter */
  {
    int nPrimaryInputs = Abc_NtkPiNum(pNtkCkt1);
    //pNtkMiter = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_AIG, 1);
    pNtkMiter->pName = Extra_UtilStrsav("Miter");
    Vec_Ptr_t * vNodeXor = Vec_PtrAlloc( nPrimaryInputs );
    vNodeXor->nSize = nPrimaryInputs;
    int i;
    for ( i = 0; i < nPrimaryInputs ; i++ )
    {
      Vec_Ptr_t * vNodePi = Vec_PtrAlloc(2);
      vNodePi->nSize = 2;
      vNodePi->pArray[0] = (void **)Abc_NtkCreatePi( pNtkMiter );
      vNodePi->pArray[1] = (void **)Abc_NtkCreatePi( pNtkMiter );
      vMiterNodePiId.push_back(Abc_ObjId((Abc_Obj_t *)vNodePi->pArray[0]));
      vMiterNodePiId.push_back(Abc_ObjId((Abc_Obj_t *)vNodePi->pArray[1]));
      vNodeXor->pArray[i] = (void **)Abc_NtkCreateNodeExor(pNtkMiter,vNodePi);
    }
    Abc_Obj_t *pObjOr = Abc_NtkCreateNodeOr(pNtkMiter,vNodeXor);
    Abc_Obj_t *pObjPo = Abc_NtkCreatePo(pNtkMiter);
    
    Abc_ObjAddFanin(pObjPo,pObjOr);
    //Vec_PtrFree(vNodeXor);
    //return 0;
    pNtkMiter = Abc_NtkStrash(pNtkMiter, 1, 0, 0);
    Aig_Man_t *pAigMan = Abc_NtkToDar(pNtkMiter, 0, 0);
    
    vMiterNodePoId.push_back(Abc_ObjId(Abc_ObjFanin0(pObjPo))) ;
    pCnfMiter = Cnf_Derive(pAigMan , 1);
  }

  LiftOffset = LiftSize(LiftOffset+Lit2Obj_ckt1->numPo*2*Lit2Obj_ckt2->numPo);
  Cnf_DataLift(pCnfMiter, LiftOffset);
  Lit2Obj *Lit2Obj_Miter =  collectLitid2Objid(pNtkMiter,pCnfMiter);


  int miterOutLitId = Lit2Obj_Miter->vec_poLitId[0];
  printf("Miter Out Lit ID: %d\n",miterOutLitId);

  sat_solver *pMatchingSat = sat_solver_new();
  
  pMatchingSat->verbosity = 2;
  pMatchingSat->fPrintClause = 1;
  int statueInt = 0;
  Cnf_DataWriteIntoSolverInt(pMatchingSat, pCnfCkt1, 1, 0);
  Cnf_DataWriteIntoSolverInt(pMatchingSat, pCnfCkt2, 1, 0);
  statueInt += vvWriteToSat(vvClausePi,pMatchingSat);
  statueInt += vvWriteToSat(&vvClausePiRule,pMatchingSat);
  statueInt += vvWriteToSat(vvClausePo,pMatchingSat);
  statueInt += vvWriteToSat(&vvClausePoRule,pMatchingSat);
  Cnf_DataWriteIntoSolverInt(pMatchingSat, pCnfMiter, 1, 0);
  statueInt += sat_solver_add_const(pMatchingSat,miterOutLitId,0);

  printf("Write Clause Status: %d\n",statueInt);

  vector<int> routeTable1D;
  routeTable2Vec(pPiTable,&routeTable1D);


  int satStatus = 1;
  int writeClauseStatus = 1;
  
  for (size_t i = 0; i < 5 && satStatus == 1 && writeClauseStatus == 1; ++i)
  {
    satStatus = sat_solver_solve_internal(pMatchingSat);
    printf("SAT Status: %d\n",satStatus);
    vector<int> vecAns = getSatAnswer(pMatchingSat,&routeTable1D);
    for (size_t i = 0; i < vecAns.size(); ++i)
    {
      printf("%d ",vecAns.at(i));
    }
    printf("\n");
    writeClauseStatus = vWriteToSat(vecAns,pMatchingSat,0);
    printf("Write Clause Status: %d\n",writeClauseStatus);
    
    //Sat_SolverPrintStats(pMatchingSat);
  }
  

return;


  

  //vvClauseaddClause(vvClause,)


  //Cnf_DataPrint(pCnfMiter, 1);



 
  //Abc_NtkAppend(Abc_Ntk_arr[0],Abc_Ntk_arr[1],1);
  //Abc_Ntk_t *pAllMiterNtk = Abc_NtkDup(Abc_Ntk_arr[0]);
  //Abc_Ntk_t *pAllMiterNtk = Abc_NtkMiter(Abc_Ntk_arr[0], Abc_Ntk_arr[1], 1, 0, 0, 0);
  //pAllMiterNtk = Abc_NtkStrash(pAllMiterNtk, 0, 0, 0);
  //Nm_ManCreateUniqueName(pNtk1->pManName, Abc_ObjId(pNtk1));
  
  
  //Cnf_Dat_t *pMiterCnf = Cnf_Derive(pAigManMiter, Abc_NtkPoNum(pAllMiterNtk));
  //sat_solver *pMiterSat = sat_solver_new();
  //Cnf_DataWriteIntoSolverInt(pMiterSat, pMiterCnf, 1, 0);





  //Abc_NtkAppend(Abc_Ntk_arr[0],Abc_Ntk_arr[1],1);
  
  //Abc_NtkShow(pNtk0, 0, 0, 0, 0, 0);
  //Abc_NtkShow(Abc_Ntk_arr[0], 0, 0, 0, 0, 0);



  return;
}

int Lsv_Command_BooleanMatching(Abc_Frame_t *pAbc, int argc, char **argv)
{

  int argv_num = 0;
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
  if (argc > 1)
  {
#if DEBUG_MESG_ENABLE
    printf("First argument: %s\n", argv[1]);
#endif
  }
  else
  {
    printf("Please type number of case 1 ~ 10 \n");
    return 0;
  }
  argv_num = atoi(argv[1]);
  Lsv_Ntk_BooleanMatching(argv_num);

  return 0;

usage:
  Abc_Print(-2, "usage: lsv_match [-h]\n");
  Abc_Print(-2, "\t        prints the nodes in the network\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}