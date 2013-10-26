#ifndef _TIMING_H_
#define _TIMING_H_

#include <iostream>
#include <fstream>
#include <common.h>
#include <deque>

#ifdef DEBUGGING
#define DEBUG(x) if (debugging_enabled) { x; }
#else
#define DEBUG(x)
#endif
#define DEBUG_MESSAGE(x) DEBUG(std::cerr << x)
//#define DEBUG_LOGVARDEF(x, y) if (x > 0.01) DEBUG(varDefLogFile << x << "\t" << y);
enum ProgressState {
  WALKING,
  MINIMIZING,
  CONCATING,
  MERGING,
  JOINING,
};

extern ProgressState progressState;
extern bool debugging_enabled;

//extern std::ofstream varDefLogFile;

class NameInst;

class ProgressBar {
  private:
    static int oldPercentage;
    static NameInst * oldNameInst;
    static ProgressState oldState;
  public:
    static int percentage;
    static NameInst * nameInst;
    static ProgressState state;

    static std::ostream & printProgBar();
    static std::ostream & printProgBar(int);
    static std::ostream & printProgBar(int, NameInst *);
    static std::ostream & printProgBar(int, NameInst *, ProgressState);
    static std::ostream & printProgBar(NameInst *, ProgressState);
    static std::ostream & printProgBar(int, ProgressState);
    static std::ostream & printProgBar(ProgressState);
    static std::ostream & printProgBar(NameInst *);
    static void setPercentage(int);
    static void setNameInst(NameInst *);
    static void setState(ProgressState);
};

#endif
