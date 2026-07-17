import React from 'react';
import { SafeAreaProvider } from 'react-native-safe-area-context';

import { TerminalScreen } from './src/screens/TerminalScreen';

function App() {
  return (
    <SafeAreaProvider>
      <TerminalScreen />
    </SafeAreaProvider>
  );
}

export default App;
