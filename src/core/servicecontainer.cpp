#include "servicecontainer.h"
#include <QDebug>

ServiceContainer* ServiceContainer::s_instance = nullptr;

ServiceContainer::ServiceContainer(QObject* parent)
    : QObject(parent)
{
}

ServiceContainer::~ServiceContainer()
{
    clear();
}

ServiceContainer* ServiceContainer::instance()
{
    if (!s_instance) {
        s_instance = new ServiceContainer();
    }
    return s_instance;
}

void ServiceContainer::cleanup()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void ServiceContainer::clear()
{
    m_services.clear();
    qDebug() << "ServiceContainer: All services cleared";
}

int ServiceContainer::serviceCount() const
{
    return m_services.size();
}