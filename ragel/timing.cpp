#include <cassert>
#include <stack>
#include <string>
#include <ctime>

#include "common.h"
#include "timing.h"
#include "parsedata.h"
#include "ragel.h"

bool debugging_enabled = false;
/* Number of frames to print in progress bar */
int numFramesToPrint = 5;
//std::ofstream varDefLogFile;

std::string ProgressStateNames[] = {"Walking", "Minimizing",
	                                  "Concating", "Merging",
                                    "Joining"};

int ProgressBar::percentage = 0;
ProgressState ProgressBar::state = WALKING;
NameInst * ProgressBar::nameInst = 0;

int ProgressBar::oldPercentage = 0;
ProgressState ProgressBar::oldState = WALKING;
NameInst * ProgressBar::oldNameInst = 0;

void ProgressBar::setPercentage(int _p) {
  percentage = _p;
}

void ProgressBar::setNameInst(NameInst *n) {
  nameInst = n;
}

void ProgressBar::setState(ProgressState s) {
  state = s;
}

std::ostream & ProgressBar::printProgBar(int percent, NameInst *n, ProgressState s) {
  ProgressBar::setPercentage(percent);
  ProgressBar::setNameInst(n);
  ProgressBar::setState(s);
  return ProgressBar::printProgBar();
}

std::ostream & ProgressBar::printProgBar(int percent, NameInst *n) {
  ProgressBar::setPercentage(percent);
  ProgressBar::setNameInst(n);
  return ProgressBar::printProgBar();
}

std::ostream & ProgressBar::printProgBar(int percent) {
  ProgressBar::setPercentage(percent);
  return ProgressBar::printProgBar();
}

std::ostream & ProgressBar::printProgBar(NameInst *n, ProgressState s) {
  ProgressBar::setNameInst(n);
  ProgressBar::setState(s);
  return ProgressBar::printProgBar();
}

std::ostream & ProgressBar::printProgBar(int percent, ProgressState s) {
  ProgressBar::setPercentage(percent);
  ProgressBar::setState(s);
  return ProgressBar::printProgBar();
}

std::ostream & ProgressBar::printProgBar(NameInst *n) {
  ProgressBar::setNameInst(n);
  return ProgressBar::printProgBar();
}

std::ostream & ProgressBar::printProgBar(ProgressState s) {
  ProgressBar::setState(s);
  return ProgressBar::printProgBar();
}

std::ostream & ProgressBar::printProgBar()
{
  if (!progressBar) return std::cout;
	std::string bar;
	NameInst * parent = nameInst;
  static time_t last_run = 0;

	std::string parentName;
	std::stack<NameInst *> parentTree;

  // Quit early if nothing changed to prevent
  // incredibly slow run time;
  if (nameInst == oldNameInst &&
    state == oldState &&
    percentage == oldPercentage) {
    return std::cout;
  }
  //Quit early if we've already published in the last
  // time period
  if (difftime(time(NULL), last_run) < 0.5) {
    return std::cout;
  }
  else {
    last_run = time(NULL);
  }
	while (parent) {
	  if (parent->name) {
	    parentTree.push(parent);
	  }
	  parent = parent->parent;
	}

	for(int i = 0; i < 50; i++){
	  if( i < (ProgressBar::percentage/2)){
	    bar.replace(i,1,"=");
	  }else if( i == (ProgressBar::percentage/2)){
	    bar.replace(i,1,">");
	  }else{
	    bar.replace(i,1," ");
	  }
	}
	std::cout << "\e[?25l"; /* hide the cursor */
	std::cout << "\r" "[" << bar << "] ";
	std::cout.width( 3 );
	std::cout << ProgressBar::percentage << "%     " << ProgressStateNames[ProgressBar::state];
	std::cout << "    \t";
	for (int i = 0; i < numFramesToPrint; ++i) {
	  if (parentTree.empty()) break;
	  parent = parentTree.top();
	  parentTree.pop();
	  parentName = std::string(parent->name);
	  if (i < numFramesToPrint - 1 && !parentTree.empty()) {
	    std::cout << parent->name << " > ";
	  }
	  else {
	    std::cout << parent->name;
	  }
	}
	std::cout << "\t\t\t";
	std::cout << std::flush;
	std::cout << "\e[?25h"; /* show the cursor */

  oldPercentage = percentage;
  oldNameInst = nameInst;
  oldState = state;
  return std::cout;
}

