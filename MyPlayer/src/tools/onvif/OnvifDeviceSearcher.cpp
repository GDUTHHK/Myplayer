//#include "OnvifDeviceSearcher.h"
//#include "DiscoveryMatch.h"
//#include "OnvifDeviceModel.h"
//
//#include <QDebug>
//#include <QCoreApplication>
//#include <QUrl>
//#include <QUrlQuery>
//#include <QTimer>
//
//// OnvifDeviceSearcher 实现
//OnvifDeviceSearcher::OnvifDeviceSearcher(QObject* parent)
//    : QObject(parent)
//    , m_discovery(nullptr)
//    , m_isSearching(false)
//{
//    m_discovery = new OnvifDiscovery();
//    connect(m_discovery, &OnvifDiscovery::Match, this, &OnvifDeviceSearcher::onDeviceFound);
//}
//
//OnvifDeviceSearcher::~OnvifDeviceSearcher() {
//    stopSearch();
//}
//
////void OnvifDeviceSearcher::start_Search(int timeoutMs) {
////
////}
//
//
//
//void OnvifDeviceSearcher::stopSearch() {
//    if (m_discovery && m_isSearching) {
//        m_discovery->Stop();
//        m_isSearching = false;
//    }
//}
//
//QList<DiscoveryMatch> OnvifDeviceSearcher::getFoundDevices() const {
//    return m_foundDevices;
//}
//
//void OnvifDeviceSearcher::clearDevices() {
//    m_foundDevices.clear();
//}
//
//bool OnvifDeviceSearcher::isSearching() const {
//    return m_isSearching;
//}
//
//void OnvifDeviceSearcher::onDeviceFound(const DiscoveryMatch& match) {
//    // 检查是否已经存在相同的设备
//    bool exists = false;
//    for (const auto& device : m_foundDevices) {
//        if (device == match) {
//            exists = true;
//            break;
//        }
//    }
//
//    if (!exists) {
//        m_foundDevices.append(match);
//        emit deviceFound(match);
//    }
//}
//
//void OnvifDeviceSearcher::onDiscoveryFinished() {
//    m_isSearching = false;
//    emit searchFinished(m_foundDevices.size());
//}