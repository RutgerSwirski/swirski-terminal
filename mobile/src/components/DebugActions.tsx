import React from 'react';
import { StyleSheet, View } from 'react-native';
import { Button, Card, CardContent, CardTitle } from '@swirski/ui/native';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';

type DebugActionsProps = {
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  onPing(): void;
  onSendTestNotificationSnapshot(device: Device): void;
  onSendTestNotificationReceived(device: Device): void;
  onSendTestMusicState(device: Device): void;
};

export function DebugActions({
  connectedDevice,
  connectionStatus,
  onPing,
  onSendTestNotificationSnapshot,
  onSendTestNotificationReceived,
  onSendTestMusicState,
}: DebugActionsProps) {
  if (!connectedDevice) {
    return null;
  }

  const disabled = connectionStatus !== 'ready';

  return (
    <Card variant="flat" tone="black" style={styles.card}>
      <CardContent>
        <CardTitle size="sm" tone="inverted">
          Debug actions
        </CardTitle>

        <View style={styles.actions}>
          <Button variant="outline" onPress={onPing} disabled={disabled}>
            Ping!
          </Button>

          <Button
            variant="outline"
            onPress={() => onSendTestNotificationSnapshot(connectedDevice)}
            disabled={disabled}
          >
            Send test notification snapshot
          </Button>

          <Button
            variant="outline"
            onPress={() => onSendTestNotificationReceived(connectedDevice)}
            disabled={disabled}
          >
            Send test notification received
          </Button>

          <Button
            variant="outline"
            onPress={() => onSendTestMusicState(connectedDevice)}
            disabled={disabled}
          >
            Send test music state
          </Button>
        </View>
      </CardContent>
    </Card>
  );
}

const styles = StyleSheet.create({
  card: {
    width: '100%',
  },
  actions: {
    gap: 8,
    marginTop: 12,
  },
});
