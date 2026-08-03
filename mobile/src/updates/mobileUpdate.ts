export const MOBILE_UPDATE_MANIFEST_URL =
  'https://github.com/RutgerSwirski/swirski-terminal/releases/download/mobile-latest/mobile-update.json';

export type MobileUpdateManifest = {
  versionCode: number;
  versionName: string;
  apkUrl: string;
  releaseUrl: string;
  builtAt: string;
};

export function parseMobileUpdateManifest(
  value: unknown,
): MobileUpdateManifest | null {
  if (!value || typeof value !== 'object') {
    return null;
  }

  const manifest = value as Record<string, unknown>;

  if (
    !Number.isInteger(manifest.versionCode) ||
    (manifest.versionCode as number) <= 0 ||
    typeof manifest.versionName !== 'string' ||
    typeof manifest.apkUrl !== 'string' ||
    typeof manifest.releaseUrl !== 'string' ||
    typeof manifest.builtAt !== 'string'
  ) {
    return null;
  }

  return manifest as MobileUpdateManifest;
}

export function isNewerMobileBuild(
  currentVersionCode: number,
  latestVersionCode: number,
): boolean {
  return latestVersionCode > currentVersionCode;
}
