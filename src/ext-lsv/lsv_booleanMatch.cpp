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
//#include "booleanMatching.h"


#define DUMP_BUS_GROUP_INFO 0
#define READ_BLIFFILE 1



#define METHOD2
#define SAT_SOLVE_MAX_ITERATION_SIZE 99999999
#define ENABLE_LEARNING 1
#define DISABLE_PARTIAL_ASSIGNMENT 0
#define PRINT_INFO 1
#define PRINT_CNF_INFO 0

using namespace std;

// # define READ_PIPO_GROUP

#define LiftSize(x) ((x + 1))

extern "C"
{
  void Abc_NtkShow(Abc_Ntk_t *pNtk0, int fGateNames, int fSeq, int fUseReverse, int fKeepDot, int fAigIds);
  Abc_Ntk_t *Io_ReadBlifAsAig(char *pFileName, int fCheck);
  Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters);
  Abc_Ntk_t *Abc_NtkDouble(Abc_Ntk_t *pNtk);
  void Abc_NtkMiterPrepare(Abc_Ntk_t *pNtk1, Abc_Ntk_t *pNtk2, Abc_Ntk_t *pNtkMiter, int fComb, int nPartSize, int fMulti);
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

void readBusGroup(char *filePath, vector<vector<string>> &busGroup1, vector<vector<string>> &busGroup2)
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
    if (lineCnt == 0 || lineCnt == lineOffset + 1)
      ;
    else if (lineCnt == 1 || lineCnt == lineOffset + 1 + 1)
    {
      // printf("Line: %s\n", line);
      // printf("busNum: %d\n", busNum);
      lineOffset = atoi(line) + 1;
      groupCnt = 0;
      circuitCnt++;
    }
    else
    {
      elementCnt = 0;
      char *token = strtok(line, " \n");

      {
        // printf("\tGroup ele cnt %d\n", atoi(token));
        if (circuitCnt == 1)
          busGroup1.push_back(vector<string>());
        else
          busGroup2.push_back(vector<string>());
      }
      elementCnt++;

      while (token != NULL)
      {
        // printf("\t\tToken: %s\n", token);
        if (elementCnt != 1)
        {
          if (circuitCnt == 1)
            busGroup1[groupCnt].push_back(token);
          else
            busGroup2[groupCnt].push_back(token);
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
void b()
{
  int a = 6;
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
void extracBusGroupVector(vector<vector<string>> &InputStringVector, map<int, string> &OutputVector)
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
void findPiPoNodeId(Abc_Ntk_t *pNtk, map<int, string> &lookupTable)
{
  Abc_Obj_t *pObj;
  int i;

  Abc_NtkForEachPi(pNtk, pObj, i)
  {
    char *nodeName = Abc_ObjName(pObj);
    int nodeId = Abc_ObjId(pObj);
    lookupTable[nodeId] = nodeName;
  }
  Abc_NtkForEachPo(pNtk, pObj, i)
  {
    char *nodeName = Abc_ObjName(pObj);
    int nodeId = Abc_ObjId(pObj);
    lookupTable[nodeId] = nodeName;
  }
}
void matchBusIdName(map<int, string> loookupTable, vector<vector<int>> &busGroupId, vector<vector<string>> &busGroupName)
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
int getSupportNum(Abc_Obj_t *pObj)
{
  Abc_Obj_t **vPObj = new Abc_Obj_t *[1];
  vPObj = &pObj;
  Vec_Ptr_t *vPSupportObj;
  vPSupportObj = Abc_NtkNodeSupport(pObj->pNtk, vPObj, 1);
  int supportNum = Vec_PtrSize(vPSupportObj);
  Vec_PtrFree(vPSupportObj);
  return supportNum;
}
void renameObj(Abc_Ntk_t *pNtk, char *circuitName)
{
  Abc_Obj_t *pObj;
  int i;
  Abc_NtkForEachObj(pNtk, pObj, i)
  {

    char *nodeName = Abc_ObjName(pObj);
    char newName[64];
    sprintf(newName, "%s_%s", circuitName, nodeName);

    printf("Old obj Name: %s ID: %d \t", Abc_ObjName(pObj), Abc_ObjId(pObj));
    Abc_ObjAssignName(pObj, newName, NULL);
    printf("New obj Name: %s ID: %d\n", Abc_ObjName(pObj), Abc_ObjId(pObj));
  }
}
void PrintObjName(Abc_Ntk_t *pNtk)
{
  Abc_Obj_t *pObj;
  int i;
  Abc_NtkForEachObj(pNtk, pObj, i)
  {
    printf("Obj name: %s \t id:%d\n", Abc_ObjName(pObj), Abc_ObjId(pObj));
  }
}

//* Boolean matching */



typedef struct LIT2OBJ
{
  int numPi;
  int numPo;
  vector<int> vec_piObjId;
  vector<int> vec_piLitId;
  vector<int> vec_poObjId;
  vector<int> vec_poLitId;
  vector<int> vec_poLitInv;
} Lit2Obj;

typedef struct RouteTable_t
{
  int **ppTable;
  int *pRow;
  int *pCol;
  int nRow;
  int nCol;
} RouteTable;

typedef struct supportNode_t
{
  int headObjId;
  int headCnfId;
  int nSupportSize;
  vector<int> vSupportObjId;
  vector<int> vSupportCnfId;

  int headCnfValue;
  vector<int> vSupportCnfValue;

  // Just for convenience to store the value of the support node

} supportNode;




std::vector<simNode>  simulation_vsimNode(Abc_Ntk_t *pNtk, u_int64_t patterns)
{
  if(!Abc_NtkIsStrash(pNtk))
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
  
  int nodeNumber = Abc_NtkObjNum(pNtk);
  //printf("Node Number: %d\n",nodeNumber);
  std::vector<simNode> simNodeArray(nodeNumber+1);
  Abc_Obj_t *IterateObj;
  int obj_ite_num = 0;


  Abc_NtkForEachObj(pNtk, IterateObj, obj_ite_num)
  {
    int nodeId = Abc_ObjId(IterateObj);
    simNodeArray[nodeId].nodeValue = 2;
    //printf("Node ID: %d Node type: %d\n",nodeId,Abc_ObjType(IterateObj));
    if(Abc_ObjType(IterateObj) == ABC_OBJ_PI)
    {
      int shiftDigit = nodeId - 1;
      bool boolNodeValue = (patterns & (1 << shiftDigit))>>shiftDigit;
      simNodeArray[nodeId].nodeValue = boolNodeValue;
      //printf("\t Node value: %d\n",simNodeArray[nodeId].nodeValue ); 
    }
    if(Abc_ObjType(IterateObj) != ABC_OBJ_NODE)
      continue;
    
    unsigned int faninId[2] = {Abc_ObjId(Abc_ObjFanin0(IterateObj)), Abc_ObjId(Abc_ObjFanin1(IterateObj))};
    //printf("Fanin0 ID: %d, Fanin1 ID: %d\n",faninId[0],faninId[1]);
    int faninInv[2] = {Abc_ObjFaninC0(IterateObj), Abc_ObjFaninC1(IterateObj)};
    int faninVal[2] = {(simNodeArray[faninId[0]].nodeValue ^ faninInv[0]),(simNodeArray[faninId[1]].nodeValue ^ faninInv[1])};
    simNodeArray[nodeId].nodeValue = faninVal[0] & faninVal[1];

    
  }

  int PoCntIndex = 0;
  int outputSum = 0;
  Abc_NtkForEachPo(pNtk, IterateObj, obj_ite_num)
  {

    int nodeId = Abc_ObjId(IterateObj);
    //printf("Po id :%d",nodeId); 
    int faninId = Abc_ObjId(Abc_ObjFanin0(IterateObj));
    int faninInv = Abc_ObjFaninC0(IterateObj);
    int faninVal = simNodeArray[faninId].nodeValue ^ faninInv;
    simNodeArray[nodeId].nodeValue = faninVal;
    //printf("Po id :%d, Po value: %d\n",nodeId,faninVal);
    outputSum += faninVal<<PoCntIndex++;
    
  }
return simNodeArray;
}

void partialAssigment(Abc_Ntk_t * pNtk,Abc_Obj_t * pObj,std::vector<simNode>  &vecSimNode,std::vector<int> * partialSetObjId)
{
  int nodeId = Abc_ObjId(pObj);
  //printf("@ ID: %d\n", nodeId);
  if(vecSimNode[nodeId].traversed == 1)
  {
    //printf("Node ID: %d is already traversed\n", nodeId);
    return;
  }
  if(Abc_ObjIsPi(pObj))
  {

    partialSetObjId->push_back(nodeId); 
    return;
  }
  else{
    vecSimNode[nodeId].traversed = 1;
    int nodeValue = vecSimNode[nodeId].nodeValue;
    int fanin0Value = vecSimNode[Abc_ObjId(Abc_ObjFanin0(pObj)) ].nodeValue ^ Abc_ObjFaninC0(pObj);
    int fanin1Value = 2;
    if(!Abc_ObjIsPo(pObj))
      fanin1Value = vecSimNode[Abc_ObjId(Abc_ObjFanin1(pObj)) ].nodeValue ^ Abc_ObjFaninC1(pObj);
    int fanin0SupportNum = getSupportNum(Abc_ObjFanin0(pObj));
    int fanin1SupportNum = fanin0SupportNum + 100;
    if(!Abc_ObjIsPo(pObj))
      fanin1SupportNum = getSupportNum(Abc_ObjFanin1(pObj));
    int fanin0Control = nodeValue == 1 && fanin0Value == 1;
    int fanin1Control = 2;
    if(!Abc_ObjIsPo(pObj))
      fanin1Control = nodeValue == 1 && fanin1Value == 1;
/*
    printf("fanin0Value: %d ^ %d => %d\n",vecSimNode[Abc_ObjId(Abc_ObjFanin0(pObj)) ].nodeValue,Abc_ObjFaninC0(pObj), fanin0Value);
    if(!Abc_ObjIsPo(pObj))
      printf("fanin1Value: %d ^ %d => %d\n",vecSimNode[Abc_ObjId(Abc_ObjFanin1(pObj)) ].nodeValue,Abc_ObjFaninC1(pObj), fanin1Value);
    printf("\n");



*/
    if(Abc_ObjIsPo(pObj))
    {
      partialAssigment(pNtk,Abc_ObjFanin0(pObj),vecSimNode,partialSetObjId);
    }
    else if(nodeValue == 0 && fanin0Control == 0 && fanin1Control == 0)
    {
      if(fanin0SupportNum > fanin1SupportNum)
        partialAssigment(pNtk,Abc_ObjFanin1(pObj),vecSimNode,partialSetObjId);
      else
        partialAssigment(pNtk,Abc_ObjFanin0(pObj),vecSimNode,partialSetObjId);

    }
    else if(nodeValue == 0 && fanin0Control == 0)
    {
        partialAssigment(pNtk,Abc_ObjFanin0(pObj),vecSimNode,partialSetObjId);

    }
    else if(nodeValue == 0 && fanin1Control == 0)
    {
        partialAssigment(pNtk,Abc_ObjFanin1(pObj),vecSimNode,partialSetObjId);
    }
    else if(nodeValue == 1 && fanin0Control == 1 && fanin1Control == 1)
    {
        partialAssigment(pNtk,Abc_ObjFanin0(pObj),vecSimNode,partialSetObjId);
        partialAssigment(pNtk,Abc_ObjFanin1(pObj),vecSimNode,partialSetObjId);
    }
    else
    {

        printf("Out of condition!!!!!!!!!!!!!!!!!!!!!!!!\n");   


    }
  }
}

vector<int>  vecBlockAns(vector<int> * vecAns){
  vector<int>  vecBlockAns;
  for (int i = 0; i < vecAns->size(); i++)
  {
    vecBlockAns.push_back(-vecAns->at(i));
  }
  return vecBlockAns;
}



int vvClauseaddClause(vector<vector<int>> *vvClause, vector<int> vClause)
{
  vvClause->push_back(vClause);
  return 0;
}



void createRouteTableRule(RouteTable_t *pTable, vector<vector<int>> *vvConstraint1)
{

  for (int i = 0; i < pTable->nCol; i += 2)
  {
    vector<int> vvSubArray;
    for (int j = 0; j < pTable->nRow; j++)
    {
      vvSubArray.push_back(pTable->ppTable[j][i]);
      vvSubArray.push_back(pTable->ppTable[j][i + 1]);
    }
    vvConstraint1->push_back(vvSubArray);
  }
  for (int i = 0; i < pTable->nRow; i++)
  {
    vector<int> vvSubArray;
    for (int j = 0; j < pTable->nCol; j++)
    {
      vvSubArray.push_back(pTable->ppTable[i][j]);
    }
    vvConstraint1->push_back(vvSubArray);
  }
}

void routeTable2Vec(RouteTable_t *pTable, vector<int> *vvConstraint1)
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

void converRouteTable2Clauses(vector<vector<int>> *vvConstraint1, vector<vector<int>> *vvClause)
{
  //* Make each line contain exact one "1" in the table

  for (size_t i = 0; i < vvConstraint1->size(); ++i)
  {
    vector<int> vLine = vvConstraint1->at(i);
    vvClauseaddClause(vvClause, vLine);
    for (size_t base_index = 0; base_index < vLine.size(); base_index++)
    {

      for (size_t move_index = base_index + 1; move_index < vLine.size(); move_index++)
      {
        vector<int> vClause;
        vClause.push_back(-vLine[base_index]);
        vClause.push_back(-vLine[move_index]);
        vvClauseaddClause(vvClause, vClause);
        // for (int val : vClause) printf("%d ", val);printf("\n");
      }
    }
    // printf("\n");
  }
}

vector<vector<int>> *createRouteTable(RouteTable_t *pTable, int litStartFrom, int numX, int numY, vector<int> xIndex, vector<int> yIndex) //,
{
  //! xIndex and yIndex will connect to each other and control by route table

  vector<vector<int>> *vvClause = new vector<vector<int>>;
  
  pTable->nRow = numY;
  pTable->nCol = 2 * numX;
  pTable->ppTable = new int *[pTable->nRow];
  for (int y_ite = 0; y_ite < pTable->nRow; ++y_ite)
  {
    pTable->ppTable[y_ite] = new int[pTable->nCol];
    for (int x_ite = 0; x_ite < pTable->nCol; ++x_ite)
    {
      pTable->ppTable[y_ite][x_ite] = y_ite * pTable->nCol + x_ite + litStartFrom;

      //* When xIndex[x_ite] is negative, clause is -|x| -|y| which -|x| => -(-x) =>x
      //* and y remains the same
      vector<int> vClause;
      vClause = {-pTable->ppTable[y_ite][x_ite], -xIndex[x_ite], yIndex[y_ite]};
      vvClause->push_back(vClause);

      vClause = {-pTable->ppTable[y_ite][x_ite], xIndex[x_ite], -yIndex[y_ite]};
      vvClause->push_back(vClause);
    }
  }
  return vvClause;
}

Lit2Obj *collectLitid2Objid(Abc_Ntk_t *pNtk, Cnf_Dat_t *pCnf)
{
  Lit2Obj *Lit2Obj_1 = new Lit2Obj;
  Lit2Obj_1->numPi = Abc_NtkPiNum(pNtk);
  Lit2Obj_1->numPo = Abc_NtkPoNum(pNtk);

  Lit2Obj_1->vec_piLitId.resize(Lit2Obj_1->numPi);
  Lit2Obj_1->vec_piObjId.resize(Lit2Obj_1->numPi);
  Lit2Obj_1->vec_poLitId.resize(Lit2Obj_1->numPo);
  Lit2Obj_1->vec_poObjId.resize(Lit2Obj_1->numPo);
  Lit2Obj_1->vec_poLitInv.resize(Lit2Obj_1->numPo);

  Abc_Obj_t *pObj;
  int i;
  Abc_NtkForEachPi(pNtk, pObj, i)
  {
    Lit2Obj_1->vec_piLitId[i] = pCnf->pVarNums[Abc_ObjId(pObj)];
    Lit2Obj_1->vec_piObjId[i] = Abc_ObjId(pObj);
  }
  Abc_NtkForEachPo(pNtk, pObj, i)
  {

    // printf("@PO: %d \t",Abc_ObjId(pObj));
    Abc_Obj_t *pObjTmp = pObj;
    int litId = pCnf->pVarNums[Abc_ObjId(pObjTmp)];
    int inv = 0;
    while (litId == -1)
    {
      inv = inv ^ pObjTmp->fCompl0;
      //printf("@%2d \tinv:%d\n", i, pObjTmp->fCompl0);
      pObjTmp = Abc_ObjFanin0(pObjTmp);
      litId = pCnf->pVarNums[Abc_ObjId(pObjTmp)];
      // printf("Get Fanin ID instead: %d\n",Abc_ObjId(pObjTmp));
    }

    Lit2Obj_1->vec_poLitId[i] = litId;
    Lit2Obj_1->vec_poLitInv[i] = inv;
    Lit2Obj_1->vec_poObjId[i] = Abc_ObjId(pObj);
  }
  return Lit2Obj_1;
}
#if PRINT_INFO == 0
  void printRouteTable(RouteTable *s, int row, int col){}
  void printAtLine(int line){}
  void printLit2Obj(Lit2Obj *Lit2Obj_1,std::string sPrintName){}
  void printSupportNode(supportNode *_supportNode){}
  void printVv(vector<vector<int>> *vvClause){}
#else

void printRouteTable(RouteTable *s, int row, int col)
{
  printf("===============================  Route Table ==========================\n");
  for (int i = 0; i < row; i++)
  {
    for (int j = 0; j < col; j++)
    {
      printf("%4d ", s->ppTable[i][j]);
    }
    printf("\n");
  }
  printf("========================================================================\n");
}
void printAtLine(int line)
{

    printf("\t\t\t\t\t\t@ Code Line: %d\n", line);
  
}
void printLit2Obj(Lit2Obj *Lit2Obj_1,std::string sPrintName)
{
  printf("+++++++++++ Lit2Obj %15s +++++++++++++++\n",sPrintName.c_str());
  printf("\tPI Lit ID (%d):\n\t",Lit2Obj_1->numPi);
  for (size_t i = 0; i < Lit2Obj_1->vec_piLitId.size(); ++i)
  {
    printf("%d ", Lit2Obj_1->vec_piLitId[i]);
  }
  printf("\n\t");
  printf("PI Obj ID (%d):\n\t",Lit2Obj_1->numPi);
  for (size_t i = 0; i < Lit2Obj_1->vec_piObjId.size(); ++i)
  {
    printf("%d ", Lit2Obj_1->vec_piObjId[i]);
  }
  printf("\n\t");
  printf("PO Lit ID (%d):\n\t",Lit2Obj_1->numPo);
  for (size_t i = 0; i < Lit2Obj_1->vec_poLitId.size(); ++i)
  {
    printf("%d ", Lit2Obj_1->vec_poLitId[i]);
  }
  printf("\n\t");
  printf("PO Obj ID (%d):\n\t",Lit2Obj_1->numPo);
  for (size_t i = 0; i < Lit2Obj_1->vec_poObjId.size(); ++i)
  {
    printf("%d ", Lit2Obj_1->vec_poObjId[i]);
  }
  printf("\n");
  printf("++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

}
void printSupportNode(supportNode *_supportNode)
{

  printf("--------------------------- Support Node ---------------------------\n");
  // printf("  objId: %d\n", _supportNode->headObjId);
  printf("  cnfId: %d\n", _supportNode->headCnfId);
  printf("  cnfValue: %d\n", _supportNode->headCnfValue);
  printf("  nSupport: %d\n", _supportNode->nSupportSize);
  // printf("  vSupportObjId: ");
  // for (size_t k = 0; k < _supportNode->vSupportObjId.size(); ++k)
  //{
  // printf("%d ", _supportNode->vSupportObjId[k]);
  // }
  printf("\n  vSupportCnfId: ");
  for (size_t k = 0; k < _supportNode->vSupportCnfId.size(); ++k)
  {
    printf("%d ", _supportNode->vSupportCnfId[k]);
  }
  printf("\n  vSupportCnfValue: ");
  for (size_t k = 0; k < _supportNode->vSupportCnfValue.size(); ++k)
  {
    printf("%d ", _supportNode->vSupportCnfValue[k]);
  }
  printf("\n");
  printf("--------------------------------------------------------------------\n");
}
void printVv(vector<vector<int>> *vvClause)
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
#endif


int vvWriteToSat(vector<vector<int>> *vvClause, sat_solver *sat, int fBlockAns)
{
  for (size_t i = 0; i < vvClause->size(); ++i)
  {
    vector<int> vLitClause;
    for (size_t j = 0; j < vvClause->at(i).size(); ++j)
    {
      int var = abs((vvClause->at(i).at(j)));
      int sign = vvClause->at(i).at(j) < 0 ? 1 ^ fBlockAns : 0 ^ fBlockAns;
      vLitClause.push_back(Abc_Var2Lit(var, sign));
    }
    if (!sat_solver_addclause(sat, vLitClause.data(), vLitClause.data() + vLitClause.size()))
    {

      return 0;
    }
  }
  return 1;
}
int vWriteToSat(vector<int> vLitClause, sat_solver *sat, int fBlockAns)
{
  vector<int> vClause_;
  for (size_t i = 0; i < vLitClause.size(); ++i)
  {
    int lit = abs((vLitClause.at(i)));
    int sign = vLitClause.at(i) < 0 ? 1 : 0;
    vClause_.push_back(Abc_Var2Lit(lit, sign) ^ fBlockAns);
  }
  if (sat_solver_addclause(sat, vClause_.data(), vClause_.data() + vClause_.size()) != 1)
  {
    return 0;
  }
  return 1;
}
int vWriteToSat(vector<int> vVarClause, vector<int> vSign, sat_solver *sat, int fBlockAns)
{
  vector<int> vClause_;
  for (size_t i = 0; i < vVarClause.size(); ++i)
  {

    vClause_.push_back(Abc_Var2Lit(vVarClause.at(i), vSign.at(i)) ^ fBlockAns);
  }
  if (sat_solver_addclause(sat, vClause_.data(), vClause_.data() + vClause_.size()) != 1)
  {
    return 0;
  }
  return 1;
}


vector<int> getSatAnswer(sat_solver *sat, vector<int> *filter)
{
  vector<int> vecResult; // = new vector<int>;
  for (size_t i = 0; i < filter->size(); ++i)
  {
    // printf("%d ",filter->at(i),sat_solver_var_value(sat, filter->at(i)));
    int lit = filter->at(i);
    int sign = sat_solver_var_value(sat, lit);
    lit = sign ? lit : -lit;
    vecResult.push_back(lit);
  }
  return vecResult;
}

vector<int> RouteTable2Vec(RouteTable *pTable)
{
  vector<int> vvConstraint1;
  for (int i = 0; i < pTable->nRow; ++i)
  {
    for (int j = 0; j < pTable->nCol; ++j)
    {
      vvConstraint1.push_back(pTable->ppTable[i][j]);
    }
  }
  return vvConstraint1;
}

int cnfCompare(int a, int b)
{
  if ((a < 0 && b < 0) || (a > 0 && b > 0))
    return 1;
  else
    return 0;
}

void Lsv_Ntk_BooleanMatching(int testCase)
{

  //************************************************************ 
  //*
  //*                     Read ckt1 and ckt2 from file
  //*
  //************************************************************

  char groupFilePath[64] = {};
  sprintf(groupFilePath, "release/case%02d/input", testCase);
  vector<vector<string>> busGroup1Name;
  vector<vector<string>> busGroup2Name;
#ifdef READ_PIPO_GROUP
  map<int, string> busGroup1Map;
  map<int, string> busGroup2Map;

  readBusGroup(groupFilePath, busGroup1Name, busGroup2Name);

  extracBusGroupVector(busGroup1Name, busGroup1Map);
  extracBusGroupVector(busGroup2Name, busGroup2Map);
#endif
  //* Read Circuit file */

  char filePath[2][128] = {};
  Abc_Ntk_t **Abc_Ntk_arr_aig = ABC_ALLOC(Abc_Ntk_t *, 2);
  Abc_Ntk_t **Abc_Ntk_arr = ABC_ALLOC(Abc_Ntk_t *, 2);

#if READ_BLIFFILE
  sprintf(filePath[0], "release/case%02d/circuit_1.blif", testCase);
  sprintf(filePath[1], "release/case%02d/circuit_1.blif", testCase);
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
  map<int, string> lookupTable1;
  map<int, string> lookupTable2;

  findPiPoNodeId(Abc_Ntk_arr[0], lookupTable1);
  findPiPoNodeId(Abc_Ntk_arr[1], lookupTable2);

  //* Match Bus ID and Name */
  vector<vector<int>> busGroup1Id;
  vector<vector<int>> busGroup2Id;

  matchBusIdName(lookupTable1, busGroup1Id, busGroup1Name);
  matchBusIdName(lookupTable2, busGroup2Id, busGroup2Name);

  busGroup1Name.clear();
  busGroup2Name.clear();
  busGroup1Map.clear();
  busGroup2Map.clear();
  lookupTable1.clear();
  lookupTable2.clear();

  // dumpBusGroup(busGroup1Id);
  // dumpBusGroup(busGroup2Id);

  // Measure time of the following process

  renameObj(Abc_Ntk_arr[1], "ckt2");
  renameObj(Abc_Ntk_arr[0], "ckt1");

  PrintObjName(Abc_Ntk_arr[0]);
  PrintObjName(Abc_Ntk_arr[1]);

#endif

  //************************************************************ 
  //*
  //*                     Create CNF of Ckt1
  //*
  //************************************************************


  Abc_Ntk_t *pNtkCkt1 = Abc_NtkDup(Abc_Ntk_arr[0]);
  Aig_Man_t *pManCkt1 = Abc_NtkToDar(pNtkCkt1, 0, 0);
  Cnf_Dat_t *pCnfCkt1 = Cnf_Derive(pManCkt1, Abc_NtkPoNum(pNtkCkt1));
 
  int maxLitCkt1 = pCnfCkt1->nVars;
  Lit2Obj *Lit2Obj_ckt1 = collectLitid2Objid(pNtkCkt1, pCnfCkt1);

  vector<supportNode> supportNode_ckt1(Lit2Obj_ckt1->numPo);
  Vec_Ptr_t *vSupport_ckt1;
  Abc_Obj_t **pObjArr_ckt1 = new Abc_Obj_t *[1];
  Abc_Obj_t *pObj_ckt1;
  int numPo_ckt1 = Abc_NtkPoNum(pNtkCkt1);
  int i_ckt1;

#ifdef PRINT_CNF_INFO
  printf("MaxLit ckt1: %d\n",maxLitCkt1);
  printf("Circuit 1 CNF:\n");
  Cnf_DataPrint(pCnfCkt1, 1);
#endif
  //* Find support node in cnf form */ 

  Abc_NtkForEachPo(pNtkCkt1, pObj_ckt1, i_ckt1)
  {
    pObjArr_ckt1[0] = pObj_ckt1;
    vSupport_ckt1 = Abc_NtkNodeSupport(pNtkCkt1, pObjArr_ckt1, 1);
    supportNode_ckt1[i_ckt1].headObjId = Abc_ObjId(pObj_ckt1);
    for (size_t j = 0; j < Vec_PtrSize(vSupport_ckt1); j++)
      supportNode_ckt1[i_ckt1].vSupportObjId.push_back(Abc_ObjId((Abc_Obj_t *)Vec_PtrEntry(vSupport_ckt1, j)));
    std::sort(supportNode_ckt1[i_ckt1].vSupportObjId.begin(), supportNode_ckt1[i_ckt1].vSupportObjId.end());
  
  }
  // Find vSupportCnfId by checking Lit2Obj_ckt1
  for (i_ckt1 = 0; i_ckt1 < numPo_ckt1; ++i_ckt1)
  {
    for (size_t j = 0; j < supportNode_ckt1[i_ckt1].vSupportObjId.size(); ++j)
    {
      int supportObjId = supportNode_ckt1[i_ckt1].vSupportObjId[j];
      for (size_t k = 0; k < Lit2Obj_ckt1->vec_piObjId.size(); ++k)
      {
        if (Lit2Obj_ckt1->vec_piObjId[k] == supportObjId)
        {
          supportNode_ckt1[i_ckt1].vSupportCnfId.push_back(Lit2Obj_ckt1->vec_piLitId[k]);
          break;
        }
      }
    }
  }
  for (i_ckt1 = 0; i_ckt1 < numPo_ckt1; ++i_ckt1)
  {
    for (int j = 0; j < Lit2Obj_ckt1->numPi; ++j)
    {
      if (Lit2Obj_ckt1->vec_poObjId[j] == supportNode_ckt1[i_ckt1].headObjId)
      {
        supportNode_ckt1[i_ckt1].headCnfId = Lit2Obj_ckt1->vec_poLitId[j];
        break;
      }
    }
  }




  //************************************************************ 
  //*
  //*                     Create CNF of Ckt2
  //*
  //************************************************************





  Abc_Ntk_t *pNtkCkt2 = Abc_NtkDup(Abc_Ntk_arr[1]);
  Aig_Man_t *pManCkt2 = Abc_NtkToDar(pNtkCkt2, 0, 0);
  Cnf_Dat_t *pCnfCkt2 = Cnf_Derive(pManCkt2, Abc_NtkPoNum(pNtkCkt2));
  int maxLitCkt2 = pCnfCkt2->nVars;
  int LiftOffset = LiftSize(maxLitCkt1);


  Cnf_DataLift(pCnfCkt2, LiftOffset);
  Lit2Obj *Lit2Obj_ckt2 = collectLitid2Objid(pNtkCkt2, pCnfCkt2);

#ifdef PRINT_CNF_INFO
  printf("MaxLit ckt2: %d\n",maxLitCkt2);
  printf("Circuit 2 CNF:\n");
  Cnf_DataPrint(pCnfCkt2, 1);
#endif

  //printf("Circuit 2 CNF:\n");
  //Cnf_DataPrint(pCnfCkt1, 1);

  vector<supportNode> supportNode_ckt2(Lit2Obj_ckt2->numPo);

  Vec_Ptr_t *vSupport_ckt2;
  Abc_Obj_t **pObjArr_ckt2 = new Abc_Obj_t *[1];
  Abc_Obj_t *pObj_ckt2;
  int numPo_ckt2 = Abc_NtkPoNum(pNtkCkt2);
  int i_ckt2 = 0;


  //* Find support node in cnf form */

  Abc_NtkForEachPo(pNtkCkt2, pObj_ckt2, i_ckt2)
  {
    printAtLine(920);
    pObjArr_ckt2[0] = pObj_ckt2;
    vSupport_ckt2 = Abc_NtkNodeSupport(pNtkCkt2, pObjArr_ckt2, 1);
    supportNode_ckt2[i_ckt2].headObjId = Abc_ObjId(pObj_ckt2);
    for (size_t j = 0; j < Vec_PtrSize(vSupport_ckt2); j++)
      {
        int vSupportObjIdElement = Abc_ObjId((Abc_Obj_t *)Vec_PtrEntry(vSupport_ckt2, j));
        supportNode_ckt2[i_ckt2].vSupportObjId.push_back(vSupportObjIdElement);}
    std::sort(supportNode_ckt2[i_ckt2].vSupportObjId.begin(), supportNode_ckt2[i_ckt2].vSupportObjId.end());
  }

  // Find vSupportCnfId by checking Lit2Obj_ckt2
  for (i_ckt2 = 0; i_ckt2 < numPo_ckt2; ++i_ckt2)
  {
    for (size_t j = 0; j < supportNode_ckt2[i_ckt2].vSupportObjId.size(); ++j)
    {
      int supportObjId = supportNode_ckt2[i_ckt2].vSupportObjId[j];
      for (size_t k = 0; k < Lit2Obj_ckt2->vec_piObjId.size(); ++k)
      {
        if (Lit2Obj_ckt2->vec_piObjId[k] == supportObjId)
        {
          supportNode_ckt2[i_ckt2].vSupportCnfId.push_back(Lit2Obj_ckt2->vec_piLitId[k]);
          break;
        }
      }
    }
  }
  for (i_ckt2 = 0; i_ckt2 < numPo_ckt2; ++i_ckt2)
  {
    for (int j = 0; j < Lit2Obj_ckt2->numPi; ++j)
    {
      if (Lit2Obj_ckt2->vec_poObjId[j] == supportNode_ckt2[i_ckt2].headObjId)
      {
        supportNode_ckt2[i_ckt2].headCnfId = Lit2Obj_ckt2->vec_poLitId[j];
        break;
      }
    }
  }




  //************************************************************ 
  //*
  //*                     Create Route Table of PIs
  //*
  //************************************************************




  //* Create xIndex and yIndex of PIs */
  vector<int> ckt1PiArr(Lit2Obj_ckt1->numPi * 2);
  vector<int> ckt2PiArr(Lit2Obj_ckt2->numPi);

  for (int i = 0; i < Lit2Obj_ckt1->numPi; i++)
  {
    ckt1PiArr[2 * i] = Lit2Obj_ckt1->vec_piLitId[i];
    ckt1PiArr[2 * i + 1] = -Lit2Obj_ckt1->vec_piLitId[i];
  }

  for (int i = 0; i < Lit2Obj_ckt2->numPi; i++)
    ckt2PiArr[i] = Lit2Obj_ckt2->vec_piLitId[i];


  // * Create PI permutation and negation table */

  
  LiftOffset = LiftSize(LiftOffset + maxLitCkt2);
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
  createRouteTableRule(pPiTable, &vvPiTableRule);
  converRouteTable2Clauses(&vvPiTableRule, &vvClausePiRule);
  vvPiTableRule.clear();



  //************************************************************ 
  //*
  //*                     Create Miter
  //*
  //************************************************************




  Abc_Ntk_t *pNtkMiter = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_AIG, 1);
  vector<int> vMiterNodePiId;
  vector<int> vMiterNodePoId;

  Cnf_Dat_t *pCnfMiter; 

  
  
  int nPrimaryOutputs = Abc_NtkPoNum(pNtkCkt1);
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
  LiftOffset = LiftSize(LiftOffset + Lit2Obj_ckt1->numPi * 2 * Lit2Obj_ckt2->numPi);
  Cnf_DataLift(pCnfMiter, LiftOffset);
  Lit2Obj *Lit2Obj_Miter = collectLitid2Objid(pNtkMiter, pCnfMiter);
  #ifdef PRINT_CNF_INFO
  printf("MaxLit miter: %d\n",pCnfMiter->nVars);
  printf("Miter CNF:\n");
  Cnf_DataPrint(pCnfMiter, 1);
#endif

  
  int miterOutLitId = Lit2Obj_Miter->vec_poLitId[0];

  
  printLit2Obj(Lit2Obj_Miter,"Lit2Obj_Miter");




  //************************************************************ 
  //*
  //*                     Create Route Table of POs
  //*
  //************************************************************




  //* Create xIndex and yIndex of pos */

  //* xIndex => ckt2Po
  //* yIndex => miter right fanins


  vector<int> ckt2PoArr(Lit2Obj_ckt2->numPo * 2);
  vector<int> miterFanin1Arr(Lit2Obj_Miter->numPi/2);


  for (int i = 0; i < Lit2Obj_ckt2->numPo; i++){
    ckt2PoArr[i << 1]     = Lit2Obj_ckt2->vec_poLitId   [(i)];
    ckt2PoArr[(i << 1)+1] = -Lit2Obj_ckt2->vec_poLitId  [(i)];
  }
  for (int i = 0; i < Lit2Obj_Miter->numPi >> 1; i++){
    miterFanin1Arr[i] = Lit2Obj_Miter->vec_piLitId[(i<<1) + 1];
  }







  // * Create PO permutation and negation table */
  LiftOffset = LiftSize(LiftOffset + pCnfMiter->nVars);
  RouteTable *pPoTable = new RouteTable;
  vector<vector<int>> *vvClausePo = createRouteTable(
      pPoTable,
      LiftOffset,
      Lit2Obj_ckt1->numPo,
      Lit2Obj_ckt2->numPo,
      ckt2PoArr,
      miterFanin1Arr);

  // vvPrint(vvClausePo);
  printRouteTable(pPoTable, pPoTable->nRow, pPoTable->nCol);
  vector<vector<int>> vvPoTableRule;
  vector<vector<int>> vvClausePoRule;
  createRouteTableRule(pPoTable, &vvPoTableRule);
  converRouteTable2Clauses(&vvPoTableRule, &vvClausePoRule);


  printVv(vvClausePo);
  vvPoTableRule.clear();



  //***************************************************************************** 
  //*
  //*                     Connect Miter Xor fanin0 to Pos of Ckt1
  //*
  //*****************************************************************************

    vector<vector<int>> * vvClause_miter2ckt1 = new vector<vector<int>>;
    for (int i = 0; i < Lit2Obj_ckt1->numPo; i++)
    {
      int ck1ItePo        = Lit2Obj_ckt1->vec_poLitId[i];
      int miterIteFanin0  = Lit2Obj_Miter->vec_piLitId[2*i];
      vector<int> vConnectClause_1 = {ck1ItePo,-miterIteFanin0};
      vector<int> vConnectClause_2 = {-ck1ItePo,miterIteFanin0};
      vvClause_miter2ckt1->push_back(vConnectClause_1);
      vvClause_miter2ckt1->push_back(vConnectClause_2); 
    }
    
    /*
    printf("pCnfCkt1:\n");
    Cnf_DataPrint(pCnfCkt1, 1);
    printf("pCnfCkt2:\n");
    Cnf_DataPrint(pCnfCkt2, 1);

    printf("vvClause_miter2ckt1:\n");
    printVv(vvClause_miter2ckt1);

    printLit2Obj(Lit2Obj_ckt1,"Lit2Obj_ckt1");

    printLit2Obj(Lit2Obj_Miter,"Lit2Obj_Miter");
    */









  //***************************************************************************** 
  //*
  //*                     Sat Solver Initialization 
  //*
  //*****************************************************************************


  

  sat_solver *pMatchingSat = sat_solver_new();

  pMatchingSat->verbosity = 0;
  pMatchingSat->fPrintClause = 0;
  int statueInt = 0;

  Cnf_DataWriteIntoSolverInt(pMatchingSat, pCnfCkt1, 1, 0);
  Cnf_DataWriteIntoSolverInt(pMatchingSat, pCnfCkt2, 1, 0);
  statueInt += vvWriteToSat(vvClausePi, pMatchingSat, 0);
  statueInt += vvWriteToSat(&vvClausePiRule, pMatchingSat, 0);
  statueInt += vvWriteToSat(vvClausePo, pMatchingSat, 0);
  statueInt += vvWriteToSat(&vvClausePoRule, pMatchingSat, 0);
  Cnf_DataWriteIntoSolverInt(pMatchingSat, pCnfMiter, 1, 0);
  pMatchingSat->fPrintClause = 1;

  // pMatchingSat->assigns[miterOutLitId] = 0;

  statueInt += sat_solver_add_const(pMatchingSat, miterOutLitId, Lit2Obj_Miter->vec_poLitInv[0]);

  vector<int> piRouteTable1D  = RouteTable2Vec(pPiTable);
  vector<int> poRouteTable1D  = RouteTable2Vec(pPoTable);
  vector<int> routeTable1D;
  vector<int> ckt1PioCnf       = Lit2Obj_ckt1->vec_piLitId;
  ckt1PioCnf.insert(ckt1PioCnf.end(), Lit2Obj_ckt1->vec_poLitId.begin(), Lit2Obj_ckt1->vec_poLitId.end());
  vector<int> ckt2PioCnf       = Lit2Obj_ckt2->vec_piLitId;
  ckt2PioCnf.insert(ckt2PioCnf.end(), Lit2Obj_ckt2->vec_poLitId.begin(), Lit2Obj_ckt2->vec_poLitId.end());


  routeTable1D.insert(routeTable1D.end(), piRouteTable1D.begin(), piRouteTable1D.end());
  routeTable1D.insert(routeTable1D.end(), poRouteTable1D.begin(), poRouteTable1D.end());

  int satStatus = 1;
  int writeClauseStatus = 2;
  pMatchingSat->fPrintClause = 0;
  //pMatchingSat->random_seed = time(NULL);
  vector<vector<int>> vvUnsatClause;
  size_t sat_cnt = 0;





  //***************************************************************************** 
  //*
  //*                     Sat Solver Solving 
  //*
  //*****************************************************************************







  for (sat_cnt = 0; sat_cnt < SAT_SOLVE_MAX_ITERATION_SIZE && satStatus == 1 && writeClauseStatus == 2; ++sat_cnt)
  {


    printf("SAT Count: %zu\n", sat_cnt);
    //system("clear");
    writeClauseStatus = 0;
    satStatus = sat_solver_solve_internal(pMatchingSat);
    if(satStatus != 1)
      break;
    vector<int> vecSatPiTable = getSatAnswer(pMatchingSat, &piRouteTable1D);
    vector<int> vecSatPoTable = getSatAnswer(pMatchingSat, &poRouteTable1D);
    vector<int> vecSatisfyAns = getSatAnswer(pMatchingSat, &routeTable1D);
    
    vector<int> vecSatCkt1Pio = getSatAnswer(pMatchingSat, &ckt1PioCnf);
    vector<int> vecSatCkt2Pio = getSatAnswer(pMatchingSat, &ckt2PioCnf);
#if PRINT_INFO
    printf("\n\n\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("\t\t\t\t New Iteration\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("vecSatCkt1Pio: ");
    for (size_t i = 0; i < vecSatCkt1Pio.size(); ++i)
    {
      printf("%d ", vecSatCkt1Pio[i]);
    }
    printf("\n");

    printf("Ckt1 Pio Obj: ");
    for (size_t i = 0; i < Lit2Obj_ckt1->vec_piObjId.size(); ++i)
    {
      printf("%d ", Lit2Obj_ckt1->vec_piObjId[i]);
    }
    printf("\n");

    printf("vecSatCkt2Pio: ");
    for (size_t i = 0; i < vecSatCkt2Pio.size(); ++i)
    {
      printf("%d ", vecSatCkt2Pio[i]);
    }
    printf("\n");

    printf("Ckt2 Pio Obj: ");
    for (size_t i = 0; i < Lit2Obj_ckt2->vec_piObjId.size(); ++i)
    {
      printf("%d ", Lit2Obj_ckt2->vec_piObjId[i]);
    }
    printf("\n");
#endif
    /*
    vector<int> vecAnsIndex;
    for (int i = 1; i < pMatchingSat->size; ++i)
    {
      vecAnsIndex.push_back(i);
    }
    vector<int> vecAns = getSatAnswer(pMatchingSat, &vecAnsIndex);
    */
#if PRINT_INFO && 0
    printf("vecAns: ");
    for (size_t i = 0; i < vecAns.size(); ++i)
    {
      printf("%d ", vecAns[i]);
    }
    printf("\n");

    printf("Ckt 1 Pi : ");
    for (size_t i = 0; i < ckt1PiArr.size(); ++i)
    {
      printf("%d ", ckt1PiArr[i]);
    }
    printf("\n");
    printf("Ckt 2 Pi : ");
    for (size_t i = 0; i < ckt2PiArr.size(); ++i)
    {
      printf("%d ", ckt2PiArr[i]);
    }
    printf("\n");
    printf("Ckt 1 Po : ");
    for (size_t i = 0; i < ckt1PoArr.size(); ++i)
    {
      printf("%d ", ckt1PoArr[i]);
    }
    printf("\n");
    printf("Ckt 2 Pi : ");
    for (size_t i = 0; i < ckt2PoArr.size(); ++i)
    {
      printf("%d ", ckt2PoArr[i]);
    }
    printf("\n");
#endif
    // printf("Miter Out: %d\n",vecAnsmiterId[0]);
    //* writeClauseStatus = vWriteToSat(routeTable1D,vecAns,pMatchingSat,0);

    // supportNode *supportNode_ckt1 = new supportNode[Lit2Obj_ckt1->numPo];
    // supportNode *supportNode_ckt2 = new supportNode[Lit2Obj_ckt2->numPo];

    //* get support node value of each po in both ckt1 & ckt2
    for (size_t j = 0; j < Lit2Obj_ckt1->numPo; ++j)
    {
      vector<int> vSupportCnfValue = getSatAnswer(pMatchingSat, &supportNode_ckt1[j].vSupportCnfId);
      supportNode_ckt1[j].vSupportCnfValue = vSupportCnfValue;
      vector<int> vDummyCnfHead = {supportNode_ckt1[j].headCnfId};
      vector<int> vHeadCnfValue = getSatAnswer(pMatchingSat, &vDummyCnfHead);
      supportNode_ckt1[j].headCnfValue = vHeadCnfValue[0];
    }
    for (size_t j = 0; j < Lit2Obj_ckt2->numPo; ++j)
    {
      vector<int> vSupportCnfValue = getSatAnswer(pMatchingSat, &supportNode_ckt2[j].vSupportCnfId);
      //std::reverse(vSupportCnfValue.begin(),vSupportCnfValue.end());
      supportNode_ckt2[j].vSupportCnfValue = vSupportCnfValue;
      vector<int> vDummyCnfHead = {supportNode_ckt2[j].headCnfId};
      vector<int> vHeadCnfValue = getSatAnswer(pMatchingSat, &vDummyCnfHead);
      supportNode_ckt2[j].headCnfValue = vHeadCnfValue[0];
    }



    vector<int> vCkt1PiValue = getSatAnswer(pMatchingSat, &Lit2Obj_ckt1->vec_piLitId);
    vector<int> vCkt2PiValue = getSatAnswer(pMatchingSat, &Lit2Obj_ckt2->vec_piLitId);

    //* convert cnf value to obj value

    map<int,int> vCkt1_Id_Value;
    map<int,int> vCkt2_Id_Value;
    
      for(size_t k = 0; k < Lit2Obj_ckt1->numPi; ++k)
      {
        int val = vCkt1PiValue[k]; 
        int lit = Lit2Obj_ckt1->vec_piLitId[k];
        int obj = Lit2Obj_ckt1->vec_piObjId[k];
        //* printf("@1  Lit: %d Val: %d Obj: %d\n",lit,val,obj);
        vCkt1_Id_Value[obj] = val>0?1:0;
      }
      for(size_t k = 0; k < Lit2Obj_ckt2->numPi; ++k)
      {
        int val = vCkt2PiValue[k]; 
        int lit = Lit2Obj_ckt2->vec_piLitId[k];
        int obj = Lit2Obj_ckt2->vec_piObjId[k];
        //* printf("@2  Lit: %d Val: %d Obj: %d\n",lit,val,obj);
        vCkt2_Id_Value[obj] = val>0?1:0;
      }


    //* obj values to pattern values
    int ckt1_cnt = 0;
    int ckt2_cnt = 0;

    u_int64_t ckt1Pattern = 0;
    u_int64_t ckt2Pattern = 0;
    for (const auto &pair : vCkt1_Id_Value)
    {
      int objId = pair.first;
      int value = pair.second;
      ckt1Pattern |= (value << (objId - 1));
    }
    for (const auto &pair : vCkt2_Id_Value)
    {
      int objId = pair.first;
      int value = pair.second;
      ckt2Pattern |= (value << (objId - 1));
    }
    //* printf("Ckt1 Pattern: %d\n", ckt1Pattern);
    //* printf("Ckt2 Pattern: %d\n", ckt2Pattern);

    //* simulation
    std::vector<int> ckt1PartialSetCnfId;
    std::vector<int> ckt2PartialSetCnfId;
# if DISABLE_PARTIAL_ASSIGNMENT == 0
    std::vector<simNode>  ckt1SimNode = simulation_vsimNode(pNtkCkt1, ckt1Pattern);
    std::vector<simNode>  ckt2SimNode = simulation_vsimNode(pNtkCkt2, ckt2Pattern);

    std::vector<int> *ckt1PartialSetObjId = new std::vector<int>;
    std::vector<int> *ckt2PartialSetObjId = new std::vector<int>;


    
    
    Abc_NtkForEachPo(pNtkCkt1, pObj_ckt1, i_ckt1)
    {
       partialAssigment(pNtkCkt1,pObj_ckt1 ,ckt1SimNode,ckt1PartialSetObjId);
    }
    //Remove duplicates from ckt1PartialSetObjId
  // Remove duplicates from ckt1PartialSetObjId
    sort(ckt1PartialSetObjId->begin(), ckt1PartialSetObjId->end());
    ckt1PartialSetObjId->erase(unique(ckt1PartialSetObjId->begin(), ckt1PartialSetObjId->end()), ckt1PartialSetObjId->end());
    
    Abc_NtkForEachPo(pNtkCkt2, pObj_ckt2, i_ckt2)
    {
       partialAssigment(pNtkCkt2,pObj_ckt2 ,ckt2SimNode,ckt2PartialSetObjId);
    }
    // Remove duplicates from ckt1PartialSetObjId
    sort(ckt2PartialSetObjId->begin(), ckt2PartialSetObjId->end());
    ckt2PartialSetObjId->erase(unique(ckt2PartialSetObjId->begin(), ckt2PartialSetObjId->end()), ckt2PartialSetObjId->end());


  //* printf("ckt1PartialSetObjId size: %d\n", ckt1PartialSetObjId->size());
  for(int j = 0; j < ckt1PartialSetObjId->size(); ++j)
  {
    //* printf("%d ",ckt1PartialSetObjId->at(j));
  }
  //* printf("\n");
  //* printf("ckt2PartialSetObjId size: %d\n", ckt2PartialSetObjId->size());
  for(int j = 0; j < ckt2PartialSetObjId->size(); ++j)
  {
    //* printf("%d ",ckt2PartialSetObjId->at(j));
  }
  //* printf("\n");


  


    for (int objId : *ckt1PartialSetObjId)
    {
      for (size_t k = 0; k < Lit2Obj_ckt1->vec_piObjId.size(); ++k)
      {
        if (Lit2Obj_ckt1->vec_piObjId[k] == objId)
        {
          ckt1PartialSetCnfId.push_back(Lit2Obj_ckt1->vec_piLitId[k]);
          break;
        }
      }
    }

    for (int objId : *ckt2PartialSetObjId)
    {
      for (size_t k = 0; k < Lit2Obj_ckt2->vec_piObjId.size(); ++k)
      {
        if (Lit2Obj_ckt2->vec_piObjId[k] == objId)
        {
          ckt2PartialSetCnfId.push_back(Lit2Obj_ckt2->vec_piLitId[k]);
          break;
        }
      }
    }



#endif




#if PRINT_INFO
    for (size_t j = 0; j < Lit2Obj_ckt1->numPo; ++j)
      printSupportNode(&supportNode_ckt1[j]);
    for (size_t j = 0; j < Lit2Obj_ckt1->numPo; ++j)
      printSupportNode(&supportNode_ckt2[j]);
#endif

    //* supportNode_ckt2 ==> g
    //* supportNode_ckt1 ==> f

    vector<vector<int>> vvLearnClause;
    for (size_t k = 0; k < supportNode_ckt1.size(); k++)
    {
      for (size_t l = 0; l < supportNode_ckt2.size(); l++)
      {
        //* clause create here
        vector<int> vLearnClause;
        int c_kl = pPoTable->ppTable[l][2 * k];
        int d_kl = pPoTable->ppTable[l][2 * k + 1];
        if (!cnfCompare(supportNode_ckt1[k].headCnfValue, supportNode_ckt2[l].headCnfValue))
        {
           vLearnClause.push_back(-c_kl);
          //* vLearnClause.push_back(d_kl);
        }
        else
        {
          vLearnClause.push_back(-d_kl);
          //* vLearnClause.push_back(c_kl);

        }

        //* v
        //* for (size_t j = supportNode_ckt2[l].vSupportCnfId.size() -1 ;j>=0 ; j--)
         for (size_t j = 0; j < supportNode_ckt2[l].vSupportCnfId.size(); j++)
        {
          //* u
          for (size_t i = 0; i < supportNode_ckt1[k].vSupportCnfId.size(); i++)
          {
            
            int u = supportNode_ckt1[k].vSupportCnfValue[i];
            int v = supportNode_ckt2[l].vSupportCnfValue[j];
            #if PRINT_INFO
            printf("Compare ckt1Pi[%d]:%d\tckt2Pi[%d]:%d    ==> %d \t",i,u,j,v,cnfCompare(u,v));
            #endif

            if (((find(ckt1PartialSetCnfId.begin(),ckt1PartialSetCnfId.end(), abs(u))!= ckt1PartialSetCnfId.end()) 
                && 
                (find(ckt2PartialSetCnfId.begin(), ckt2PartialSetCnfId.end(), abs(v))!= ckt2PartialSetCnfId.end())) 
                || DISABLE_PARTIAL_ASSIGNMENT)
            {
              if (!cnfCompare(u, v))
              {
            #if PRINT_INFO

                 printf("A_%d%d ",j,i);
            #endif
                vLearnClause.push_back(pPiTable->ppTable[j][2 * i ]);
              }
              else
              {
            #if PRINT_INFO

                  printf("B_%d%d ",j,i);
            #endif
                vLearnClause.push_back(pPiTable->ppTable[j][2 * i + 1]);
              }
            }else
            {
              //* printf("_|_\n");
            }
            #if PRINT_INFO
            printf("\n");
            #endif


            

          }
                  

        }
            #if PRINT_INFO
printf("\n");
            #endif
        /*
        printf("vLearnClause: ");
        for (size_t i = 0; i < vLearnClause.size(); ++i)
        {
          printf("%d ", vLearnClause[i]);
        }
        printf("\n");
*/
        #if DISABLE_PARTIAL_ASSIGNMENT == 0 && 0
        printf("ckt1PartialSetCnfId: ");
        for (size_t i = 0; i < ckt1PartialSetCnfId.size(); ++i)
        {
          printf("%d ", ckt1PartialSetCnfId[i]);
        }
        printf("\n");
        printf("ckt2PartialSetCnfId: ");
        for (size_t i = 0; i < ckt2PartialSetCnfId.size(); ++i)
        {
          printf("%d ", ckt2PartialSetCnfId[i]);
        }
        printf("\n");
        #endif
if(1)
{
#if ENABLE_LEARNING
        vvUnsatClause.push_back(vLearnClause);
#endif
        vvLearnClause.push_back(vLearnClause);}
      }
    }

#if ENABLE_LEARNING
    writeClauseStatus += vvWriteToSat(&vvLearnClause, pMatchingSat, 0);
    
#else
    writeClauseStatus++;
#endif
    //* vvUnsatClause.push_back(vecBlockAns(&vecSatisfyAns));
    writeClauseStatus += vWriteToSat(vecSatisfyAns, pMatchingSat, 1);
  }
  printf("Sat solver stop!\n");
  printf("SAT Status: %d\n", satStatus);
  printf("Write Clause Status: %d\n", writeClauseStatus);
  printf("SAT Count: %d\n", sat_cnt);
  //sat_solver_delete(pMatchingSat);

  //=======================================================================================================
  //
  //                      New SAT Solver for the second part
  //
  //=======================================================================================================

  sat_solver *pMatchingSat_2 = sat_solver_new();
  #if PRINT_INFO
  pMatchingSat_2->verbosity = 2;
  pMatchingSat_2->fPrintClause = 1;
  #endif
  statueInt = 0;

  Cnf_DataWriteIntoSolverInt(pMatchingSat_2, pCnfCkt1, 1, 0);
  Cnf_DataWriteIntoSolverInt(pMatchingSat_2, pCnfCkt2, 1, 0);
  statueInt += vvWriteToSat(vvClausePi, pMatchingSat_2, 0);
  statueInt += vvWriteToSat(&vvClausePiRule, pMatchingSat_2, 0);
  statueInt += vvWriteToSat(vvClausePo, pMatchingSat_2, 0);
  statueInt += vvWriteToSat(&vvClausePoRule, pMatchingSat_2, 0);
  Cnf_DataWriteIntoSolverInt(pMatchingSat_2, pCnfMiter, 1, 0);

  pMatchingSat_2->fPrintClause = 0;
  //statueInt += vvWriteToSat(&vvUnsatClause, pMatchingSat_2, 0);

  //Cnf_DataWriteIntoSolverInt(pMatchingSat_2, pCnfMiter, 1, 0);
  pMatchingSat_2->fPrintClause = 0;

  // printf("Valid Case\n\n\n\n");

  satStatus = sat_solver_solve_internal(pMatchingSat_2);
  vector<int> vecSatPiTable = getSatAnswer(pMatchingSat_2, &piRouteTable1D);
  vector<int> vecSatPoTable = getSatAnswer(pMatchingSat_2, &poRouteTable1D);
  

  printf("SAT Status: %d\n", satStatus);

  
    printRouteTable(pPoTable, pPoTable->nRow, pPoTable->nCol);
    printRouteTable(pPiTable, pPiTable->nRow, pPiTable->nCol);
    //* printf("vecSatPoTable: \n");

    int posAnsCnt = 0;
    for (size_t i = 0; i < vecSatPoTable.size(); ++i)
    {

      if (vecSatPoTable[i] > 0 )
      {
        printf("%d ", vecSatPoTable[i]);
        posAnsCnt++;
        sat_solver_add_const(pMatchingSat_2, vecSatPoTable[i], 0);
      }
    }
    //* printf("\n");
    //* printf("vecSatPiTable: \n");
    for (size_t i = 0; i < vecSatPiTable.size(); ++i)
    {
      if (vecSatPiTable[i] > 0 )
      {
        printf("%d ", vecSatPiTable[i]);
        posAnsCnt++;
        sat_solver_add_const(pMatchingSat_2, vecSatPiTable[i], 0);
      }
    }
    //* printf("\n");
  
if(posAnsCnt == 0)
{
  printf("Circuit unmatch\n");
  return;
}

  //sat_solver_add_const(pMatchingSat_2, vecSatPiTable[i], 0);
  int invertable = (Lit2Obj_Miter->vec_poLitInv[0] & 1) ^ 1;
  sat_solver_add_const(pMatchingSat, miterOutLitId, 1);
  int sat2statue = sat_solver_solve_internal(pMatchingSat_2);

  vector<int> vecAnsIndex;
  for (int i = 1; i < pMatchingSat_2->size; ++i)
  {
    vecAnsIndex.push_back(i);
  }
  vector<int> vecAns = getSatAnswer(pMatchingSat_2, &vecAnsIndex);
  if(sat2statue == 1)
  {  
    printf("Final Satisfy Ans In CNF: \n");
    for (size_t i = 0; i < vecAns.size(); ++i)
    {
      if(vecAns[i] > 0)
      {
        for (int j = 0; j < pPoTable->nRow; ++j)
        {
          for (int k = 0; k < pPoTable->nCol; ++k)
          {
            if (pPoTable->ppTable[j][k] == vecAns[i])
            {
              printf("pPoTable[%d][%d] = %d ", j, k, vecAns[i]);
                
               //Lit2Obj_ckt2->vec_poObjId ckt2PoArr[k]

              printf("{ckt1:%d,ckt2:%d}\n",miterFanin1Arr[j],ckt2PoArr[k]);
              break;
                
              
            }
          }
        }
        for (int j = 0; j < pPiTable->nRow; ++j)
        {
          for (int k = 0; k < pPiTable->nCol; ++k)
          {
            if (pPiTable->ppTable[j][k] == vecAns[i])
            {
              printf("pPiTable[%d][%d] = %d ", j, k, vecAns[i]);

               //Lit2Obj_ckt2->vec_poObjId ckt2PoArr[k]
                
              printf("{ckt1:%d,ckt2:%d}\n",ckt1PiArr[k],ckt2PiArr[j]);
              b();
            }
          }
        }
        //printf("%d ", vecAns[i]);
      }
    }
  }

  return;

  // vvClauseaddClause(vvClause,)

  // Cnf_DataPrint(pCnfMiter, 1);

  // Abc_NtkAppend(Abc_Ntk_arr[0],Abc_Ntk_arr[1],1);
  // Abc_Ntk_t *pAllMiterNtk = Abc_NtkDup(Abc_Ntk_arr[0]);
  // Abc_Ntk_t *pAllMiterNtk = Abc_NtkMiter(Abc_Ntk_arr[0], Abc_Ntk_arr[1], 1, 0, 0, 0);
  // pAllMiterNtk = Abc_NtkStrash(pAllMiterNtk, 0, 0, 0);
  // Nm_ManCreateUniqueName(pNtk1->pManName, Abc_ObjId(pNtk1));

  // Cnf_Dat_t *pMiterCnf = Cnf_Derive(pAigManMiter, Abc_NtkPoNum(pAllMiterNtk));
  // sat_solver *pMiterSat = sat_solver_new();
  // Cnf_DataWriteIntoSolverInt(pMiterSat, pMiterCnf, 1, 0);

  // Abc_NtkAppend(Abc_Ntk_arr[0],Abc_Ntk_arr[1],1);

  // Abc_NtkShow(pNtk0, 0, 0, 0, 0, 0);
  // Abc_NtkShow(Abc_Ntk_arr[0], 0, 0, 0, 0, 0);

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
    printf("Please type number of case 0 ~ 10 \n");
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