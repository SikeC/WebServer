#ifndef  __MYLOG_H_
#define __MYLOG_H_

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/Category.hh>

log4cpp::Category* mylog_init();
void pDebug(std::string str,log4cpp::Category* root);
void pInfo(std::string str,log4cpp::Category* root);
void pWarn(std::string str,log4cpp::Category* root);
void pError(std::string str,log4cpp::Category* root);
void pCrit(std::string str,log4cpp::Category* root);
void pNotice(std::string str,log4cpp::Category* root);

#endif
