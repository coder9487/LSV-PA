#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"



void printObjValInNtk(Abc_Ntk_t *pNtk){
  Abc_Obj_t *pIteObj;
  int i = 0;
  printf("==========================================\n");
  Abc_NtkForEachObj(pNtk,pIteObj,i)
  {
    printf("[@ NODE %5s]:\tid=%d\tval=%d\n",Abc_ObjName(pIteObj),Abc_ObjId(pIteObj),pIteObj->pTemp);
  }
  printf("==========================================\n");

}

int getSupportNum( Abc_Obj_t *pObj){
  Abc_Obj_t **vPObj = new Abc_Obj_t*[1];
  vPObj = &pObj;
  Vec_Ptr_t *vPSupportObj ;
  vPSupportObj = Abc_NtkNodeSupport(pObj->pNtk,vPObj,1);
  int supportNum = Vec_PtrSize(vPSupportObj);
  Vec_PtrFree(vPSupportObj);
  return supportNum;

}

int Simulation_P(Abc_Ntk_t *pNtk, u_int64_t patterns)
{
  if(!Abc_NtkIsStrash(pNtk))
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
  int nodeNumber = Abc_NtkObjNum(pNtk);
  Abc_Obj_t *IterateObj;
  int obj_ite_num = 0;

  Abc_NtkForEachObj(pNtk, IterateObj, obj_ite_num)
  {

    int nodeId = Abc_ObjId(IterateObj);
    IterateObj->pTemp = (void *)2;
    IterateObj->iData = 0;
    if(Abc_ObjType(IterateObj) == ABC_OBJ_PI)
    {
      int shiftDigit = nodeId - 1;
      bool boolNodeValue = (patterns & (1 << shiftDigit))>>shiftDigit;
      IterateObj->pTemp = (void *)boolNodeValue;
      //printf("\t\t@pi %d\t%d\n",Abc_ObjId(IterateObj),IterateObj->pTemp);

    }
    if(Abc_ObjType(IterateObj) != ABC_OBJ_NODE)
      continue;
 
    IterateObj->pTemp = (void *) (((bool)Abc_ObjFanin0(IterateObj)->pTemp ^ IterateObj->fCompl0) & ((bool)Abc_ObjFanin1(IterateObj)->pTemp ^ IterateObj->fCompl1));
   //printf("\t\t@node %d\t%d\n",Abc_ObjId(IterateObj),IterateObj->pTemp);
    
  }

  int PoCntIndex = 0;
  int outputSum = 0;

  Abc_NtkForEachPo(pNtk, IterateObj, obj_ite_num)
  {

    int nodeId = Abc_ObjId(IterateObj);
    int faninId = Abc_ObjId(Abc_ObjFanin0(IterateObj));
    int faninInv = Abc_ObjFaninC0(IterateObj);

    bool faninVal = (bool)Abc_ObjFanin0(IterateObj)->pTemp ^ IterateObj->fCompl0;
    IterateObj->pTemp = (void *)faninVal;
    //printf("\t\t@po %d\t%d\n",Abc_ObjId(IterateObj),IterateObj->pTemp);

    outputSum += faninVal<<PoCntIndex++;
    
  }

  
return outputSum;
}

void findPartialSet_P(Abc_Obj_t * pObj,std::vector<Abc_Obj_t *> *partialSet)
{
  int ObjId = Abc_ObjId(pObj);
  if(pObj->iData == 1)
  {
    return;
  }
    
  pObj->iData = 1;
  if(Abc_ObjIsPi(pObj))
  {
    partialSet->push_back(pObj);
    printf("\tPI: %d\n",Abc_ObjId(pObj));
    return;
  }
   
  int faninValue[2] = {
    (bool)(Abc_ObjFanin0(pObj)->pTemp )^ pObj->fCompl0,
    (bool)(Abc_ObjFanin1(pObj)->pTemp )^ pObj->fCompl1
  };

  if(Abc_ObjIsPo(pObj))
  {
    findPartialSet_P(Abc_ObjFanin0(pObj),partialSet);
  }
  else if((bool)pObj->pTemp == 0 && faninValue[0] == 0 && faninValue[1] == 0)
  {
    
    int numFaninSupp[] ={
      getSupportNum(Abc_ObjFanin0(pObj)),
      getSupportNum(Abc_ObjFanin1(pObj))
      };
      if(numFaninSupp[0] > numFaninSupp[1])
        findPartialSet_P(Abc_ObjFanin1(pObj),partialSet);
      else
        findPartialSet_P(Abc_ObjFanin0(pObj),partialSet);
  }
  else if((bool)pObj->pTemp == 0 && faninValue[0] == 0)
  {
    findPartialSet_P(Abc_ObjFanin0(pObj),partialSet);
  }
  else if((bool)pObj->pTemp == 0 && faninValue[1] == 0)
  {
    findPartialSet_P(Abc_ObjFanin1(pObj),partialSet);
  }
  else
  {
    findPartialSet_P(Abc_ObjFanin0(pObj),partialSet);
    findPartialSet_P(Abc_ObjFanin1(pObj),partialSet);
  }
  // /pObj->iTemp = 1;

  

}