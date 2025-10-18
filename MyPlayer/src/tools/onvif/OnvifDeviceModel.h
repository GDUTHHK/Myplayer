//#pragma once
//
//#include <QAbstractTableModel>
//#include <QDateTime>
//#include <QStringList>
//#include <QUrl>
//
//// 前向声明
//class DiscoveryMatch;
//
//// ONVIF设备信息结构体 - 简化版本
//struct OnvifDeviceInfo {
//    QString endpoint;           // 设备ONVIF地址
//    QString deviceName;         // 设备名称  
//    QString manufacturer;       // 制造商
//    QString model;             // 型号
//    QString location;          // 位置
//    QStringList types;         // 设备类型
//    QStringList scopes;        // 设备范围信息
//    QDateTime foundTime;       // 发现时间
//
//    OnvifDeviceInfo() = default;
//    
//    OnvifDeviceInfo(const DiscoveryMatch& match);
//
//    QString extractName() const;
//    QString extractManufacturer() const;
//    QString extractModel() const;
//    QString extractLocation() const;
//
//    bool operator==(const OnvifDeviceInfo& other) const {
//        return endpoint == other.endpoint;
//    }
//};
//
//class OnvifDeviceModel : public QAbstractTableModel {
//    Q_OBJECT
//
//public:
//    // 列枚举
//    enum Column {
//        ColumnEndpoint = 0,      // 设备地址
//        ColumnName,              // 设备名称
//        ColumnManufacturer,      // 制造商
//        ColumnModel,             // 型号
//        ColumnLocation,          // 位置
//        ColumnFoundTime,         // 发现时间
//        ColumnCount              // 列数
//    };
//
//    explicit OnvifDeviceModel(QObject* parent = nullptr);
//    virtual ~OnvifDeviceModel() = default;
//
//    // QAbstractTableModel实现的接口
//    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
//    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
//    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
//    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
//
//    // 设备操作
//    void setDevices(const QList<OnvifDeviceInfo>& devices);
//    void addDevice(const OnvifDeviceInfo& device);
//    void addDevice(const DiscoveryMatch& match);  // 添加从DiscoveryMatch转换的方法
//    void removeDevice(int row);
//    void clearDevices();
//
//    // 获取设备信息
//    OnvifDeviceInfo getDevice(int row) const;
//    QList<OnvifDeviceInfo> getAllDevices() const { return m_devices; }
//
//    // 查找设备
//    int findDevice(const QString& endpoint) const;
//    bool hasDevice(const QString& endpoint) const;
//
//    // 实用方法
//    int deviceCount() const { return m_devices.size(); }
//    bool isEmpty() const { return m_devices.isEmpty(); }
//
//signals:
//    void deviceCountChanged(int count);
//
//private:
//    QList<OnvifDeviceInfo> m_devices;
//
//    QString formatFoundTime(const QDateTime& time) const;
//    QVariant getDisplayData(const OnvifDeviceInfo& device, int column) const;
//};