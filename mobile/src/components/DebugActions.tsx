import React from 'react';
import { Button, View } from 'react-native';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';

type DebugActionsProps = {
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  onPing(): void;
  onSendTestNotificationSnapshot(device: Device): void;
  onSendTestNotificationReceived(device: Device): void;
};

export function DebugActions({
  connectedDevice,
  connectionStatus,
  onPing,
  onSendTestNotificationSnapshot,
  onSendTestNotificationReceived,
}: DebugActionsProps) {
  if (!connectedDevice) {
    return null;
  }

  const disabled = connectionStatus !== 'ready';

  return (
    <View>
      <Button title="Ping!" onPress={onPing} disabled={disabled} />

      <Button
        title="Send test notification snapshot"
        onPress={() => onSendTestNotificationSnapshot(connectedDevice)}
        disabled={disabled}
      />

      <Button
        title="Send test notification received"
        onPress={() => onSendTestNotificationReceived(connectedDevice)}
        disabled={disabled}
      />
    </View>
  );
}
