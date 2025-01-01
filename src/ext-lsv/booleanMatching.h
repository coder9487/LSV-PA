#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "sat/cnf/cnf.h"

#define PARTIALASSIGNMENT_RETURN_VECTOR 0
#define PRINT_LEARN_CLAUSE_PROCESS 1

extern "C"
{
  Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters);
  Abc_Ntk_t *Io_ReadBlifAsAig(char *pFileName, int fCheck);
}

typedef struct ObjInfo_t
{
  u_int8_t traversed;
  u_int8_t value;
  u_int8_t isCareset;

} ObjInfo;

typedef struct LIT2OBJ
{
  int numPi;
  int numPo;
  std::vector<int> vec_piObjId;
  std::vector<int> vec_piLitId;
  std::vector<int> vec_poObjId;
  std::vector<int> vec_poLitId;
  std::vector<int> vec_poLitInv;
} Lit2Obj;

typedef struct PO_ARR
{
  Abc_Obj_t *pHeadNode;
  std::vector<Abc_Obj_t *> vSupportNode;
} SuppObj;


Lit2Obj *collectLitid2Objid(Abc_Ntk_t* pNtk, Cnf_Dat_t *pCnf);


class RouteTable
{
  public:
    int cnfLitMax;
    std::vector<std::vector<int>> vvRoutePathCNF;
    std::vector<std::vector<int>> vvRouteTableRuleCNF;
    
  private:
    std::vector<int> vRouteTable1D;
    std::vector<std::vector<int>> ppTable;
    std::vector<int> vRow;
    std::vector<int> vCol;
    int nRow;
    int nCol;



public:
 
  RouteTable(std::vector<int> xIndex, std::vector<int> yIndex, int CnfLiftFrom)
  {
    this->setXArray(xIndex);
    this->setYArray(yIndex);
    this->CreateRouteTable(CnfLiftFrom);
  };

  void setXArray(std::vector<int> xIndex)
  {
    vCol = xIndex;
    nCol = vCol.size();
  };

  void setYArray(std::vector<int> yIndex)
  {
    vRow = yIndex;
    nRow = vRow.size();
  };
  void CreateRouteTable(int CnfLiftFrom)
  {
    this->checkEssential();
    this->createRouteTable(CnfLiftFrom);
    this->createRouteTableRule_atLeast1();
    this->createRouteTableRule_atMost1();
    
    // Flatten ppTable to vRouteTable1D
    vRouteTable1D.clear();
    for (const auto& row : ppTable)
    {
      vRouteTable1D.insert(vRouteTable1D.end(), row.begin(), row.end());
    }
  }
  std::vector<int> getRouteTable1D()
  {
    return this->vRouteTable1D;
  }
  void printRouteTable()
  {
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (int i = 0; i < nRow; i++)
    {
      printf("+\t");
      for (int j = 0; j < nCol; j++)
      {
        printf("%5d ", ppTable[i][j]);
      }
      printf("\n");
    }
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  
  }
  void printRouteTableRule()
  {
    
    for (size_t i = 0; i < this->vvRouteTableRuleCNF.size(); ++i)
    {
      for (size_t j = 0; j < this->vvRouteTableRuleCNF[i].size(); ++j)
      {
        printf("%4d ", this->vvRouteTableRuleCNF[i][j]);
      }
      printf("\n");
    }
  }

  int getRouteTableElement(int y, int x)
  {
    if(y < 0 || y >= nRow || x < 0 || x >= nCol)
    {
      printf("Error: Out of bound access to RouteTable\n");
      exit(1);
    }
    return this->ppTable[y][x];
  }

private:
  void checkEssential()
  {
    if (this->vCol.size() == 0)
    {
      printf("Error: X array is empty\n");
      exit(1);
    }
    if (this->vRow.size() == 0)
    {
      printf("Error: Y array is empty\n");
      exit(1);
    }
  };
  void createRouteTable(int CnfLiftFrom)
  {
    this->ppTable.resize(nRow, std::vector<int>(nCol));
    for (int y_ite = 0; y_ite < nRow; ++y_ite)
    {

      for (int x_ite = 0; x_ite < nCol; ++x_ite)
      {
        this->ppTable[y_ite][x_ite] = y_ite * nCol + x_ite + CnfLiftFrom;

        //* When xIndex[x_ite] is negative, clause is -|x| -|y| which -|x| => -(-x) =>x
        //* and y remains the same
        std::vector<int> vClause;
        vClause = {-this->ppTable[y_ite][x_ite], -this->vCol[x_ite], this->vRow[y_ite]};
        vvRoutePathCNF.push_back(vClause);
        vClause = {-this->ppTable[y_ite][x_ite], this->vCol[x_ite], -this->vRow[y_ite]};
        this->vvRoutePathCNF.push_back(vClause);
      }
    }
    this->cnfLitMax = nRow * nCol + CnfLiftFrom;
  };

  void createRouteTableRule_atLeast1()
  {
    {
      for (int i = 0; i < nCol; i += 2)
      {
        std::vector<int> vSubArray;
        for (int j = 0; j < nRow; j++)
        {
          vSubArray.push_back(this->ppTable[j][i]);
          vSubArray.push_back(this->ppTable[j][i + 1]);
        }
        this->vvRouteTableRuleCNF.push_back(vSubArray);
      }
      for (int i = 0; i < nRow; i++)
      {
        std::vector<int> vSubArray;
        for (int j = 0; j < nCol; j++)
        {
          vSubArray.push_back(this->ppTable[i][j]);
        }
        this->vvRouteTableRuleCNF.push_back(vSubArray);
      }
    }
  }
  void createRouteTableRule_atMost1()
  {
    int height = this->vvRouteTableRuleCNF.size();
    int width = height > 0 ? this->vvRouteTableRuleCNF[0].size() : 0;
    for (int i = 0; i < height; i++)
    {
      for (int j = 0; j < width; j++)
      {
        for (int k = j + 1; k < width; k++)
        {
          std::vector<int> vSubArray;
          vSubArray.push_back(-this->vvRouteTableRuleCNF[i][j]);
          vSubArray.push_back(-this->vvRouteTableRuleCNF[i][k]);
          this->vvRouteTableRuleCNF.push_back(vSubArray);
        }
      }
    }

  }

};

class CircuitMan
{

public:
  Abc_Ntk_t *pNtk;
  Cnf_Dat_t *pCnf;
  
  std::vector<SuppObj> vSuppNode;
  Lit2Obj *lit2obj;

  CircuitMan(Abc_Ntk_t *pNtk)
  {
    this->pNtk = pNtk;
    Abc_Obj_t *pObj;
    int i = 0;
    Abc_NtkForEachObj(this->pNtk, pObj, i)
    {
      ObjInfo *pObjInfo = new ObjInfo;
      pObj->pTemp = (void *)pObjInfo;
    }
  }

  void toCnf(int nLiftFrom)
  {
    if (!Abc_NtkIsStrash(pNtk))
      pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    Aig_Man_t *pAigMan =  Abc_NtkToDar(pNtk, 0, 0);
    int nOutputs = Abc_NtkPoNum(pNtk);
    this->pCnf = Cnf_Derive(pAigMan, nOutputs);
    Cnf_DataLift(this->pCnf, nLiftFrom);
    lit2obj = collectLitid2Objid(this->pNtk,this->pCnf);
  }
  //* Deprecated
  std::vector<int> *getPosObjId()
  {
    Abc_Obj_t *pIteObj;
    int i = 0;
    std::vector<int> *poObjId = new std::vector<int>;
    Abc_NtkForEachPo(pNtk, pIteObj, i)
    {
      poObjId->push_back(Abc_ObjId(pIteObj));
    }
    return poObjId;
  }

  void PartialAssignment()
  {
    Abc_Obj_t *pObj;
    int i = 0;
    Abc_NtkForEachPo(this->pNtk, pObj, i)
    {
      findPartialSet_P(pObj);
      Abc_Obj_t *pPiObj;
      int j = 0;
      int PiAssignCnt = 0;
      
      Abc_NtkForEachPi(this->pNtk, pPiObj, j)
      {
        if (((ObjInfo *)pPiObj->pTemp)->isCareset == 1)
        {
          PiAssignCnt++;
        }
      }
      if(PiAssignCnt == Abc_NtkPiNum(this->pNtk))
      {
        printf("All PI are care set\n");
        goto endOfForEachPi;
      }
      
    }
  endOfForEachPi:

    get_pos_support();
    sort_pos_supp();
    return;
  }

  int Simulation(u_int64_t patterns)
  {
    if (!Abc_NtkIsStrash(pNtk))
      pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    int nodeNumber = Abc_NtkObjNum(pNtk);
    Abc_Obj_t *IterateObj;
    int obj_ite_num = 0;

    Abc_NtkForEachObj(pNtk, IterateObj, obj_ite_num)
    {

      int nodeId = Abc_ObjId(IterateObj);
      ((ObjInfo *)(IterateObj->pTemp))->value = 2;
      ((ObjInfo *)(IterateObj->pTemp))->isCareset = 0;
      ((ObjInfo *)(IterateObj->pTemp))->traversed = 0;
      IterateObj->iData = 0;
      if (Abc_ObjType(IterateObj) == ABC_OBJ_PI)
      {
        int shiftDigit = nodeId - 1;
        bool boolNodeValue = (patterns & (1 << shiftDigit)) >> shiftDigit;
        ((ObjInfo *)(IterateObj->pTemp))->value = boolNodeValue;
        // printf("\t\t@pi %d\t%d\n",Abc_ObjId(IterateObj),IterateObj->pTemp);
      }
      if (Abc_ObjType(IterateObj) != ABC_OBJ_NODE)
        continue;

      ((ObjInfo *)IterateObj->pTemp)->value =
          ((bool)((ObjInfo *)(Abc_ObjFanin0(IterateObj)->pTemp))->value ^ IterateObj->fCompl0) & ((bool)((ObjInfo *)(Abc_ObjFanin1(IterateObj)->pTemp))->value ^ IterateObj->fCompl1);
      // printf("\t\t@node %d\t%d\n",Abc_ObjId(IterateObj),IterateObj->pTemp);
    }

    int PoCntIndex = 0;
    int outputSum = 0;

    Abc_NtkForEachPo(pNtk, IterateObj, obj_ite_num)
    {

      int nodeId = Abc_ObjId(IterateObj);
      int faninId = Abc_ObjId(Abc_ObjFanin0(IterateObj));
      int faninInv = Abc_ObjFaninC0(IterateObj);

      bool faninVal =
          (bool)((ObjInfo *)(Abc_ObjFanin0(IterateObj)->pTemp))->value ^ IterateObj->fCompl0;
      ((ObjInfo *)((IterateObj)->pTemp))->value = faninVal;
      // printf("\t\t@po %d\t%d\n",Abc_ObjId(IterateObj),faninVal);

      outputSum += faninVal << PoCntIndex++;
    }

    return outputSum;
  }

  u_int64_t Cnf2Obj2Pattern(std::vector<int> vSatCnf)
  {
    u_int64_t pattern = 0;
    for (int var : vSatCnf) {
      auto it = std::find(this->lit2obj->vec_piLitId.begin(), this->lit2obj->vec_piLitId.end(), abs(var));
      if (it != this->lit2obj->vec_piLitId.end()) {
        int index = std::distance(this->lit2obj->vec_piLitId.begin(), it);
        int objId = this->lit2obj->vec_piObjId.at(index);
        int sign = var > 0 ? 1 : 0;
        
        pattern += sign<<(objId - 1);
        
      }
    }
    return pattern;
  }


  void printObjValInNtk()
  {
    Abc_Obj_t *pIteObj;
    int i = 0;
    printf("==========================================\n");
    Abc_NtkForEachObj(this->pNtk, pIteObj, i)
    {
      u_int8_t value = ((ObjInfo *)(pIteObj->pTemp))->value;
      printf("[@ NODE %5s]:\tid=%d\tval=%d\n",Abc_ObjName(pIteObj), Abc_ObjId(pIteObj), value);
    }
    printf("==========================================\n");
  }

  void printvSuppObj()
  {
    for (size_t i = 0; i < vSuppNode.size(); ++i)
    {
      printf("[@func:printvSuppObj]\tPO ID: %d\tName:%5s \tValue:%d\n",
       Abc_ObjId(vSuppNode[i].pHeadNode),
       Abc_ObjName(vSuppNode[i].pHeadNode),
       ((ObjInfo *)vSuppNode[i].pHeadNode->pTemp)->value);
      for (size_t j = 0; j < vSuppNode[i].vSupportNode.size(); ++j)
      {
        printf("[@func:printvSuppObj]\t\tSupport: ID:%d\tName:%5s\tValue:%d\tisCare:%d\n",
         Abc_ObjId(vSuppNode[i].vSupportNode[j]),
         Abc_ObjName(vSuppNode[i].vSupportNode[j]),
        ((ObjInfo *)vSuppNode[i].vSupportNode[j]->pTemp)->value,
        ((ObjInfo *)vSuppNode[i].vSupportNode[j]->pTemp)->isCareset);
      }
    }
  }

private:


  void sort_pos_supp()
  {

    for (size_t i = 0; i < vSuppNode.size(); ++i)
    {
      std::sort(vSuppNode[i].vSupportNode.begin(), vSuppNode[i].vSupportNode.end(), [](Abc_Obj_t *a, Abc_Obj_t *b)
                { return Abc_ObjId(a) < Abc_ObjId(b); });
    }
  }

  void get_pos_support()
  {
    int i = 0;
    Abc_Obj_t *pObj;
    Vec_Ptr_t *vSupp;
    int numPo_ckt1 = Abc_NtkPoNum(this->pNtk);
    Abc_Obj_t **ppObjPo;
    std::vector<SuppObj> supportNode(numPo_ckt1);
    //* Get Po Vec_Ptr_t here */
    Abc_NtkForEachPo(this->pNtk, pObj, i)
    {
      ppObjPo = new Abc_Obj_t *[1];
      ppObjPo[0] = pObj;
      vSupp = Abc_NtkNodeSupport(this->pNtk, ppObjPo, 1);
      supportNode[i].pHeadNode = pObj;
      for (size_t j = 0; j < Vec_PtrSize(vSupp); j++)
      {
        supportNode[i].vSupportNode.push_back((Abc_Obj_t *)Vec_PtrEntry(vSupp, j));
      }
      delete[] ppObjPo;
    }
    this->vSuppNode = supportNode;
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

  void findPartialSet_P(Abc_Obj_t *pObj
#if PARTIALASSIGNMENT_RETURN_VECTOR
                        ,
                        std::vector<Abc_Obj_t *> *partialSet
#endif
  )
  {
    int ObjId = Abc_ObjId(pObj);
    if (((ObjInfo *)pObj->pTemp)->traversed == 1)
    {
      return;
    }

    ((ObjInfo *)pObj->pTemp)->traversed = 1;
    if (Abc_ObjIsPi(pObj))
    {
#if PARTIALASSIGNMENT_RETURN_VECTOR
      partialSet->push_back(pObj);
#endif
      ((ObjInfo *)pObj->pTemp)->isCareset = 1;
      printf("[@func:findPartialSet_P]\tPI: %d\n", Abc_ObjId(pObj));
      return;
    }

    int faninValue[2] = {
        //*  ((ObjInfo *)(Abc_ObjFanin0(pObj)->pTemp))->value
        (bool)((ObjInfo *)(Abc_ObjFanin0(pObj)->pTemp))->value ^ pObj->fCompl0,
        (bool)((ObjInfo *)(Abc_ObjFanin1(pObj)->pTemp))->value ^ pObj->fCompl1};

    if (Abc_ObjIsPo(pObj))
    {
#if PARTIALASSIGNMENT_RETURN_VECTOR
      findPartialSet_P(Abc_ObjFanin0(pObj), partialSet);
#else
      findPartialSet_P(Abc_ObjFanin0(pObj));
#endif
    }
    else if ((bool)((ObjInfo *)pObj->pTemp)->value == 0 && faninValue[0] == 0 && faninValue[1] == 0)
    {

      int numFaninSupp[] = {
          getSupportNum(Abc_ObjFanin0(pObj)),
          getSupportNum(Abc_ObjFanin1(pObj))};
      if (numFaninSupp[0] > numFaninSupp[1])
#if PARTIALASSIGNMENT_RETURN_VECTOR
        findPartialSet_P(Abc_ObjFanin1(pObj), partialSet);
#else
        findPartialSet_P(Abc_ObjFanin1(pObj));
#endif
      else
#if PARTIALASSIGNMENT_RETURN_VECTOR
        findPartialSet_P(Abc_ObjFanin0(pObj), partialSet);
#else
        findPartialSet_P(Abc_ObjFanin0(pObj));
#endif
    }
    else if ((bool)((ObjInfo *)pObj->pTemp)->value == 0 && faninValue[0] == 0)
    {
#if PARTIALASSIGNMENT_RETURN_VECTOR
      findPartialSet_P(Abc_ObjFanin0(pObj), partialSet);
#else
      findPartialSet_P(Abc_ObjFanin0(pObj));
#endif
    }
    else if ((bool)((ObjInfo *)pObj->pTemp)->value == 0 && faninValue[1] == 0)
    {
#if PARTIALASSIGNMENT_RETURN_VECTOR
      findPartialSet_P(Abc_ObjFanin1(pObj), partialSet);
#else
      findPartialSet_P(Abc_ObjFanin1(pObj));
#endif
    }
    else
    {
#if PARTIALASSIGNMENT_RETURN_VECTOR
      findPartialSet_P(Abc_ObjFanin0(pObj), partialSet);
      findPartialSet_P(Abc_ObjFanin1(pObj), partialSet);
#else
      findPartialSet_P(Abc_ObjFanin0(pObj));
      findPartialSet_P(Abc_ObjFanin1(pObj));
#endif
    }
    // /pObj->iTemp = 1;
  }
};


//************************************************************** */
//*
//*             Golbal Function Decalration
//*
//************************************************************** */





std::vector<std::vector<int>> LearnClause(CircuitMan *cMan1, CircuitMan *cMan2, RouteTable *pPoTable,RouteTable *pPiTable)
{
  std::vector<std::vector<int>> vvLearnClause;
  for (size_t k = 0; k < cMan1->vSuppNode.size(); ++k)
  {
    for (size_t l = 0; l < cMan2->vSuppNode.size(); ++l)
    {
      std::vector<int> vLearnClause;
      //* learn clause about c and d
      u_int8_t f_val = ((ObjInfo *)(cMan1->vSuppNode[k].pHeadNode)->pTemp)->value;
      u_int8_t g_val = ((ObjInfo *)(cMan2->vSuppNode[l].pHeadNode)->pTemp)->value;
      int c_kl = pPoTable->getRouteTableElement(l,2 * k);
      int d_kl = pPoTable->getRouteTableElement(l,2 * k + 1);

#if PRINT_LEARN_CLAUSE_PROCESS
      printf("[@func:LearnClause]\tPO Id: %d\tName:%5s\tValue:%d\n", Abc_ObjId(cMan1->vSuppNode[k].pHeadNode),Abc_ObjName(cMan1->vSuppNode[k].pHeadNode), f_val);
      printf("[@func:LearnClause]\tPO Id: %d\tName:%5s\tValue:%d\n", Abc_ObjId(cMan2->vSuppNode[l].pHeadNode),Abc_ObjName(cMan2->vSuppNode[l].pHeadNode), g_val);
#endif
      if (f_val != g_val)
      {
#if PRINT_LEARN_CLAUSE_PROCESS
        printf("\t¬C%d%d ", k, l);
        vLearnClause.push_back(-c_kl);
#endif
      }
      else
      {
#if PRINT_LEARN_CLAUSE_PROCESS
        printf("\t¬D%d%d ", k, l);
        vLearnClause.push_back(-d_kl);
#endif
      }

      for (size_t i = 0; i < cMan2->vSuppNode[l].vSupportNode.size(); ++i)
      {
        for (size_t j = 0; j < cMan1->vSuppNode[k].vSupportNode.size(); ++j)
        {
          //* learn clause about a and b
          u_int8_t f_pi_val = ((ObjInfo *)(cMan1->vSuppNode[k].vSupportNode[j])->pTemp)->value;
          u_int8_t f_pi_care = ((ObjInfo *)(cMan1->vSuppNode[k].vSupportNode[j])->pTemp)->value;
          u_int8_t g_pi_val = ((ObjInfo *)(cMan2->vSuppNode[l].vSupportNode[i])->pTemp)->value;
          u_int8_t g_pi_care = ((ObjInfo *)(cMan2->vSuppNode[l].vSupportNode[i])->pTemp)->value;
          int a_ij = pPiTable->getRouteTableElement(j,2 * i);
          int b_ij = pPiTable->getRouteTableElement(j,2 * i + 1);

          if (g_pi_care == 1 && f_pi_care == 1)
          {
            if (f_pi_val != g_pi_val)
            {
#if PRINT_LEARN_CLAUSE_PROCESS
              printf("¬A%d%d ", k, l);
              vLearnClause.push_back(a_ij);
#endif
            }
            else
            {
#if PRINT_LEARN_CLAUSE_PROCESS
              printf("¬B%d%d ", k, l);
              vLearnClause.push_back(b_ij);
#endif
            }
          }
        }
      }
      vvLearnClause.push_back(vLearnClause);
#if PRINT_LEARN_CLAUSE_PROCESS
      printf("\n");
#endif
    }
  }
  return vvLearnClause;
}


inline Lit2Obj *collectLitid2Objid(Abc_Ntk_t* pNtk, Cnf_Dat_t *pCnf)
{


  //* In Obj ID order
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











std::vector<std::vector<int>>  cnfConnection(std::vector<int> vLit1, std::vector<int> vLit2)
{
  std::vector<std::vector<int>> vvClause;
  //* negative value is illegal
  //* vLit1.size() should be equal to vLit2.size()


  if(vLit1.size() != vLit2.size())
  {
    printf("vLit1.size() should be equal to vLit2.size()\n");
    return vvClause;
  }

  for (size_t i = 0; i < vLit1.size(); i++)
  {
    if(vLit1[i] < 0 || vLit2[i] < 0)
    {
      printf("Negative value is illegal\n");
      return vvClause;
    }
    
    std::vector<int> vClause;
    vClause.push_back(vLit1[i]);
    vClause.push_back(-vLit2[i]);
    vvClause.push_back(vClause);
    vClause.clear();
    vClause.push_back(-vLit1[i]);
    vClause.push_back(vLit2[i]);
    vvClause.push_back(vClause);
  }

  return vvClause;
}









inline void printLit2Obj(Lit2Obj *Lit2Obj_1,std::string sPrintName)
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


inline std::vector<int> getSatAnswer(sat_solver *sat, std::vector<int> *filter)
{
  std::vector<int> vecResult; // = new vector<int>;
  for (size_t i = 0; i < filter->size(); ++i)
  {
    // printf("%d ",filter->at(i),sat_solver_var_value(sat, filter->at(i)));
    int var = filter->at(i);
    int sign = sat_solver_var_value(sat, var);
    var = sign ? var : -var;
    vecResult.push_back(var);
  }
  return vecResult;
}

inline int vvWriteToSat(std::vector<std::vector<int>> vvVarClause, sat_solver *sat, int fBlockAns)
{
  for (size_t i = 0; i < vvVarClause.size(); ++i)
  {
    std::vector<int> vLitClause;
    
    for (size_t j = 0; j < vvVarClause.at(i).size(); ++j)
    {
      int var = abs((vvVarClause.at(i).at(j)));
      int sign = vvVarClause.at(i).at(j) < 0 ? 1 ^ fBlockAns : 0 ^ fBlockAns;
      vLitClause.push_back(Abc_Var2Lit(var, sign));
    }
    if (!sat_solver_addclause(sat,vLitClause.data() ,vLitClause.data()  + vLitClause.size()))
    {
      return 0;
    }
  }
  return 1;
}
inline int vWriteToSat(std::vector<int> vVarClause, sat_solver *sat, int fBlockAns)
{
  std::vector<int> vClause_;
  for (size_t i = 0; i < vVarClause.size(); ++i)
  {
    int lit = abs((vVarClause.at(i)));
    int sign = vVarClause.at(i) < 0 ? 1 : 0;
    vClause_.push_back(Abc_Var2Lit(lit, sign) ^ fBlockAns);
  }
  if (sat_solver_addclause(sat, vClause_.data(), vClause_.data() + vClause_.size()) != 1)
  {
    return 0;
  }
  return 1;
}



inline void printVv(std::vector<std::vector<int>> *vvClause)
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
inline void printVv(std::vector<std::vector<int>> vvClause)
{
  for (size_t i = 0; i < vvClause.size(); ++i)
  {
    for (size_t j = 0; j < vvClause.at(i).size(); ++j)
    {
      printf("%d ", vvClause.at(i).at(j));
    }
    printf("\n");
  }
  return;
}