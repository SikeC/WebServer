#include "../include/mylog.h"

extern int errno;

log4cpp::Category* mylog_init(void)
{
    try
    {
        log4cpp::PropertyConfigurator::configure("./log4cpp.conf");
    }
    catch (log4cpp::ConfigureFailure& f)
    {
        std::cout << "Configure Problem " << f.what() << std::endl;
        return NULL;
    }
    log4cpp::Category* root = &log4cpp::Category::getRoot();
    return root;
}

void pDebug(std::string str,log4cpp::Category* root)
{
    root->debug(str.c_str());
}

void pInfo(std::string str, log4cpp::Category* root)
{
    root->info(str.c_str());
}

void pWarn(std::string str,log4cpp::Category* root)
{
    root->warn(str.c_str());
}

void pNotice(std::string str,log4cpp::Category* root)
{
    root->notice(str.c_str());
}

void pCrit(std::string str,log4cpp::Category* root)
{
    std::string errorstr;
    errorstr = strerror(errno);
    std::string tmp;
    tmp =str+errorstr;
    root->crit(tmp.c_str());
    _exit(1);
}
void pError(std::string str,log4cpp::Category* root)
{
    std::string errorstr;
    errorstr = strerror(errno);
    std::string tmp;
    tmp =str+errorstr;
    root->error(tmp.c_str());
}