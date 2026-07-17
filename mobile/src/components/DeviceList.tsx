import React from 'react';
import { Button, Text, View } from 'react-native';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';

type DeviceListProps = {
  devices: Device[];
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  onConnect(device: Device): void;
  onDisconnect(): void;
};

export function DeviceList({
  devices,
  connectedDevice,
  connectionStatus,
  onConnect,
  onDisconnect,
}: DeviceListProps) {
  return (
    <>
      {devices.map(device => (
        <View key={device.id}>
          <Text>{device.name ?? device.localName ?? 'Unnamed device'}</Text>
          <Text>{device.id}</Text>
          <Text>RSSI: {device.rssi ?? 'Unknown'}</Text>

          {connectedDevice?.id === device.id ? (
            <Button
              title={
                connectionStatus === 'disconnecting'
                  ? 'Disconnecting...'
                  : 'Disconnect'
              }
              disabled={connectionStatus === 'disconnecting'}
              onPress={onDisconnect}
            />
          ) : (
            <Button
              disabled={
                connectionStatus !== 'disconnected' &&
                connectionStatus !== 'error'
              }
              title="Connect"
              onPress={() => onConnect(device)}
            />
          )}
        </View>
      ))}
    </>
  );
}
