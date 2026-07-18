import React, { useState } from 'react';
import { StyleSheet, TextInput, View } from 'react-native';
import {
  Button,
  Card,
  CardContent,
  CardMeta,
  CardTitle,
  Text,
} from '@swirski/ui/native';

import type {
  TerminalWifiNetwork,
  TerminalWifiStatus,
} from '../wifi/useTerminalWifi';

type WifiPanelProps = {
  networks: TerminalWifiNetwork[];
  status: TerminalWifiStatus;
  onScan(): Promise<void>;
  onConnect(ssid: string, password: string): Promise<void>;
  onTestInternet(): Promise<void>;
  onDisconnect(): Promise<void>;
};

export function WifiPanel({
  networks,
  status,
  onScan,
  onConnect,
  onTestInternet,
  onDisconnect,
}: WifiPanelProps) {
  const [selectedNetwork, setSelectedNetwork] =
    useState<TerminalWifiNetwork | null>(null);
  const [password, setPassword] = useState('');

  return (
    <Card variant="outline" style={styles.card}>
      <CardContent>
        <CardTitle size="sm">Terminal Wi-Fi</CardTitle>
        <CardMeta>
          {status.state === 'connected'
            ? `Connected to ${status.ssid}`
            : status.state}
        </CardMeta>

        {status.state === 'connected' && (
          <View style={styles.internetTest}>
            <Text tone="muted">
              Internet:{' '}
              {status.internetTest === 'success'
                ? `Online (${status.internetLatencyMs} ms)`
                : status.internetTest === 'failed'
                ? 'Test failed'
                : status.internetTest === 'testing'
                ? 'Testing...'
                : 'Not tested'}
            </Text>

            <Button
              variant="outline"
              disabled={status.internetTest === 'testing'}
              onPress={onTestInternet}
            >
              Ping swirski.studio
            </Button>

            <Button tone="red" variant="outline" onPress={onDisconnect}>
              Disconnect Wi-Fi
            </Button>
          </View>
        )}

        {status.state !== 'connected' && (
          <>
            <Button disabled={status.scanning} onPress={onScan}>
              {status.scanning ? 'Scanning...' : 'Scan networks'}
            </Button>

            <View style={styles.networks}>
              {!status.scanning && networks.length === 0 && (
                <Text tone="muted">No networks found</Text>
              )}

              {networks.map(network => (
                <Button
                  key={network.ssid}
                  variant={
                    selectedNetwork?.ssid === network.ssid ? 'solid' : 'outline'
                  }
                  onPress={() => {
                    setSelectedNetwork(network);
                    setPassword('');
                  }}
                >
                  {network.ssid} {network.secured ? 'Locked' : 'Open'}
                </Button>
              ))}
            </View>
          </>
        )}

        {status.state !== 'connected' && selectedNetwork && (
          <View style={styles.configure}>
            <Text weight="bold">{selectedNetwork.ssid}</Text>

            {selectedNetwork.secured && (
              <TextInput
                autoCapitalize="none"
                autoCorrect={false}
                placeholder="Wi-Fi password"
                secureTextEntry
                style={styles.input}
                value={password}
                onChangeText={setPassword}
              />
            )}

            <Button
              disabled={selectedNetwork.secured && password.length === 0}
              onPress={async () => {
                await onConnect(selectedNetwork.ssid, password);
                setPassword('');
              }}
            >
              Connect
            </Button>
          </View>
        )}
      </CardContent>
    </Card>
  );
}

const styles = StyleSheet.create({
  card: {
    width: '100%',
  },
  networks: {
    gap: 8,
    marginTop: 12,
  },
  internetTest: {
    gap: 8,
    marginVertical: 12,
  },
  configure: {
    gap: 8,
    marginTop: 16,
  },
  input: {
    backgroundColor: '#ffffff',
    borderColor: '#111111',
    borderWidth: 2,
    color: '#111111',
    fontSize: 16,
    minHeight: 48,
    paddingHorizontal: 12,
  },
});
