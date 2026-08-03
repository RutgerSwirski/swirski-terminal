import React, { useEffect, useState } from 'react';
import {
  AppState,
  Linking,
  NativeModules,
  Platform,
  StyleSheet,
} from 'react-native';
import { Button, Card, CardContent, CardTitle, Text } from '@swirski/ui/native';

import {
  isNewerMobileBuild,
  MOBILE_UPDATE_MANIFEST_URL,
  type MobileUpdateManifest,
  parseMobileUpdateManifest,
} from '../updates/mobileUpdate';

type CurrentMobileBuild = {
  versionCode: number;
  versionName: string;
};

type SwirskiUpdatesModule = {
  getCurrentBuild(): Promise<CurrentMobileBuild>;
};

const updatesModule = NativeModules.SwirskiUpdates as
  | SwirskiUpdatesModule
  | undefined;

export function MobileUpdateCard() {
  const [update, setUpdate] = useState<MobileUpdateManifest | null>(null);
  const [currentBuild, setCurrentBuild] = useState<CurrentMobileBuild | null>(
    null,
  );

  useEffect(() => {
    let cancelled = false;

    async function checkForUpdate() {
      if (Platform.OS !== 'android' || !updatesModule) {
        return;
      }

      try {
        const installedBuild = await updatesModule.getCurrentBuild();
        const response = await fetch(
          `${MOBILE_UPDATE_MANIFEST_URL}?timestamp=${Date.now()}`,
          { headers: { Accept: 'application/json' } },
        );

        if (!response.ok) {
          throw new Error(`Update manifest returned HTTP ${response.status}`);
        }

        const manifest = parseMobileUpdateManifest(await response.json());

        if (!manifest) {
          throw new Error('Update manifest is invalid');
        }

        if (!cancelled) {
          setCurrentBuild(installedBuild);
          setUpdate(
            isNewerMobileBuild(installedBuild.versionCode, manifest.versionCode)
              ? manifest
              : null,
          );
        }
      } catch (error) {
        console.warn('Could not check for a mobile update:', error);
      }
    }

    checkForUpdate();
    const appStateSubscription = AppState.addEventListener(
      'change',
      nextState => {
        if (nextState === 'active') {
          checkForUpdate();
        }
      },
    );

    return () => {
      cancelled = true;
      appStateSubscription.remove();
    };
  }, []);

  if (!update || !currentBuild) {
    return null;
  }

  const openDownload = () => {
    Linking.openURL(update.apkUrl).catch(error => {
      console.error('Could not open the mobile update download:', error);
    });
  };

  return (
    <Card variant="outline" tone="yellow" style={styles.card}>
      <CardContent style={styles.content}>
        <CardTitle size="sm">Mobile update available</CardTitle>
        <Text tone="muted">
          Installed {currentBuild.versionName}; latest {update.versionName}.
        </Text>
        <Button onPress={openDownload}>Download update</Button>
      </CardContent>
    </Card>
  );
}

const styles = StyleSheet.create({
  card: {
    width: '100%',
  },
  content: {
    gap: 10,
  },
});
