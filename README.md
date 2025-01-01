# LSV Final-Project
Boolean matching under NPNP-equivalence. The implement approach is refer to this paper\
C. -F. Lai, J. -H. R. Jiang and K. -H. Wang, "Boolean matching of function vectors with strengthened learning," 2010 IEEE/ACM International Conference on Computer-Aided Design (ICCAD), San Jose, CA, USA, 2010, pp. 596-601, doi: 10.1109/ICCAD.2010.5654215.


## Introduction
The approach is using strong learn clause mechanism to prune impossible match in Boolean matching.

## How to run the code

### Build
In the root directory of this project. Type 
```
make 
```

### Run 
Type command  
```
lsv_match [test case]
```
And the number after lsv_match means which test case will be used in Boolean match. The test case is from ICCAD 2023 problem. 

### Output
The match port will be contain by a brace.
```
{ckt1:    [port name],ckt2:   [port name]}
```



