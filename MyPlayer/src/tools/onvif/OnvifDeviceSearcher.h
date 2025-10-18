//#pragma once
//
//#include <QObject>
//#include <QStringList>
//
//#include "OnvifDiscovery.h" 
//#include "DiscoveryMatch.h"
//
//class OnvifDeviceSearcher : public QObject {
//    Q_OBJECT
//
//public:
//    explicit OnvifDeviceSearcher(QObject* parent = nullptr);
//    ~OnvifDeviceSearcher();
//
//    // 开始搜索设备
//    void startSearch(int timeoutMs = 5000); 
//
//    // 停止搜索
//    void stopSearch();
//
//    // 获取已找到的设备列表
//    QList<DiscoveryMatch> getFoundDevices() const;
//
//    // 清空设备列表
//    void clearDevices();
//
//    // 是否正在搜索
//    bool isSearching() const;
//
//signals:
//    void deviceFound(const DiscoveryMatch& device);
//    void searchStarted();
//    void searchFinished(int deviceCount);
//    void searchError(const QString& error);
//
//private slots:
//    void onDeviceFound(const DiscoveryMatch& match);
//    void onDiscoveryFinished();
//
//private:
//    OnvifDiscovery* m_discovery;
//    QList<DiscoveryMatch> m_foundDevices;
//    bool m_isSearching;
//};