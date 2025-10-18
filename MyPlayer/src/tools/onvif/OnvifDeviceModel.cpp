//#include "OnvifDeviceModel.h"
//#include "DiscoveryMatch.h"
//
//// OnvifDeviceInfo 实现
//OnvifDeviceInfo::OnvifDeviceInfo(const DiscoveryMatch& match) {
//    QList<QUrl> endpoints = match.GetDeviceEndpoints();
//    if (!endpoints.isEmpty()) {
//        endpoint = endpoints.first().toString();
//    }
//
//    // 获取scopes信息
//    QStringList scopesList = match.GetScopes();
//    scopes = scopesList;
//
//    // 获取types信息
//    types = match.GetTypes();
//
//    // 设置发现时间
//    foundTime = QDateTime::currentDateTime();
//
//    // 提取设备信息
//    deviceName = extractName();
//    manufacturer = extractManufacturer();
//    model = extractModel();
//    location = extractLocation();
//}
//
//QString OnvifDeviceInfo::extractName() const {
//    for (const auto& scope : scopes) {
//        if (scope.contains("name/", Qt::CaseInsensitive)) {
//            return QUrl::fromPercentEncoding(scope.split('/').last().toUtf8());
//        }
//    }
//    return "Unknown Device";
//}
//
//QString OnvifDeviceInfo::extractManufacturer() const {
//    for (const auto& scope : scopes) {
//        if (scope.contains("hardware/", Qt::CaseInsensitive)) {
//            return QUrl::fromPercentEncoding(scope.split('/').last().toUtf8());
//        }
//    }
//    return "Unknown";
//}
//
//QString OnvifDeviceInfo::extractModel() const {
//    for (const auto& scope : scopes) {
//        if (scope.contains("model/", Qt::CaseInsensitive)) {
//            return QUrl::fromPercentEncoding(scope.split('/').last().toUtf8());
//        }
//    }
//    return "Unknown";
//}
//
//QString OnvifDeviceInfo::extractLocation() const {
//    for (const auto& scope : scopes) {
//        if (scope.contains("location/", Qt::CaseInsensitive)) {
//            return QUrl::fromPercentEncoding(scope.split('/').last().toUtf8());
//        }
//    }
//    return "Unknown";
//}
//
//
//
//// OnvifDeviceModel 实现
//OnvifDeviceModel::OnvifDeviceModel(QObject* parent)
//    : QAbstractTableModel(parent)
//{
//}
//
//int OnvifDeviceModel::rowCount(const QModelIndex& parent) const {
//    Q_UNUSED(parent)
//        return m_devices.size();
//}
//
//int OnvifDeviceModel::columnCount(const QModelIndex& parent) const {
//    Q_UNUSED(parent)
//        return ColumnCount;
//}
//
//QVariant OnvifDeviceModel::data(const QModelIndex& index, int role) const {
//    if (!index.isValid() || index.row() >= m_devices.size()) {
//        return QVariant();
//    }
//
//    const OnvifDeviceInfo& device = m_devices.at(index.row());
//
//    if (role == Qt::DisplayRole) {
//        return getDisplayData(device, index.column());
//    }
//
//    return QVariant();
//}
//
//QVariant OnvifDeviceModel::headerData(int section, Qt::Orientation orientation, int role) const {
//    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
//        switch (section) {
//        case ColumnEndpoint: return "设备地址";
//        case ColumnName: return "设备名称";
//        case ColumnManufacturer: return "制造商";
//        case ColumnModel: return "型号";
//        case ColumnLocation: return "位置";
//        case ColumnFoundTime: return "发现时间";
//        }
//    }
//
//    return QVariant();
//}
//
//void OnvifDeviceModel::setDevices(const QList<OnvifDeviceInfo>& devices) {
//    beginResetModel();
//    m_devices = devices;
//    endResetModel();
//    emit deviceCountChanged(m_devices.size());
//}
//
//void OnvifDeviceModel::addDevice(const OnvifDeviceInfo& device) {
//    // 检查设备是否已存在
//    if (hasDevice(device.endpoint)) {
//        return;
//    }
//
//    beginInsertRows(QModelIndex(), m_devices.size(), m_devices.size());
//    m_devices.append(device);
//    endInsertRows();
//    emit deviceCountChanged(m_devices.size());
//}
//
//void OnvifDeviceModel::addDevice(const DiscoveryMatch& match) {
//    OnvifDeviceInfo device(match);
//    addDevice(device);
//}
//
//void OnvifDeviceModel::removeDevice(int row) {
//    if (row >= 0 && row < m_devices.size()) {
//        beginRemoveRows(QModelIndex(), row, row);
//        m_devices.removeAt(row);
//        endRemoveRows();
//        emit deviceCountChanged(m_devices.size());
//    }
//}
//
//void OnvifDeviceModel::clearDevices() {
//    beginResetModel();
//    m_devices.clear();
//    endResetModel();
//    emit deviceCountChanged(0);
//}
//
//OnvifDeviceInfo OnvifDeviceModel::getDevice(int row) const {
//    if (row >= 0 && row < m_devices.size()) {
//        return m_devices.at(row);
//    }
//    return OnvifDeviceInfo();
//}
//
//int OnvifDeviceModel::findDevice(const QString& endpoint) const {
//    for (int i = 0; i < m_devices.size(); ++i) {
//        if (m_devices.at(i).endpoint == endpoint) {
//            return i;
//        }
//    }
//    return -1;
//}
//
//bool OnvifDeviceModel::hasDevice(const QString& endpoint) const {
//    return findDevice(endpoint) >= 0;
//}
//
//QString OnvifDeviceModel::formatFoundTime(const QDateTime& time) const {
//    return time.toString("hh:mm:ss");
//}
//
//QVariant OnvifDeviceModel::getDisplayData(const OnvifDeviceInfo& device, int column) const {
//    switch (column) {
//    case ColumnEndpoint: return device.endpoint;
//    case ColumnName: return device.deviceName;
//    case ColumnManufacturer: return device.manufacturer;
//    case ColumnModel: return device.model;
//    case ColumnLocation: return device.location;
//    case ColumnFoundTime: return formatFoundTime(device.foundTime);
//    }
//    return QVariant();
//}
//
//
