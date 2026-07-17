import { NativeModules } from 'react-native';

type SwirskiMediaModule = {
  handleMusicCommand(action: string): Promise<boolean>;
};

const SwirskiMedia = NativeModules.SwirskiMedia as
  | SwirskiMediaModule
  | undefined;

function readMusicCommandAction(
  message: Record<string, unknown>,
): string | null {
  if (message.type !== 'music.command') {
    return null;
  }

  const payload = message.payload;

  if (!payload || typeof payload !== 'object') {
    return null;
  }

  const action = (payload as Record<string, unknown>).action;

  return typeof action === 'string' ? action : null;
}

export async function handleMusicCommandMessage(
  message: Record<string, unknown>,
) {
  const action = readMusicCommandAction(message);

  if (!action) {
    return;
  }

  if (!SwirskiMedia) {
    console.log('Native media module is not available');
    return;
  }

  const handled =
    await SwirskiMedia.handleMusicCommand(action);

  console.log(
    handled
      ? `Handled music command: ${action}`
      : `Ignored music command: ${action}`,
  );
}
