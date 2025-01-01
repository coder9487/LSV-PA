#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "sat/cnf/cnf.h"
#include <vector>


std::vector<int> getSatAnswer(sat_solver *sat, std::vector<int> *filter)
{
  std::vector<int> vecResult; // = new vector<int>;
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