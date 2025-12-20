#!/bin/bash
# kill-screensaver-cache.sh
# 
# macOS aggressively caches screensaver bundles in System Settings.
# This script kills all related processes to force a fresh load.
# 
# ALWAYS run this script after building and installing a new version
# before testing in System Settings.

echo ">> Killing screensaver cache..."

killall "System Settings" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "  ✓ Killed System Settings"
fi

killall legacyScreenSaver 2>/dev/null
if [ $? -eq 0 ]; then
    echo "  ✓ Killed legacyScreenSaver"
fi

killall ScreenSaverEngine 2>/dev/null
if [ $? -eq 0 ]; then
    echo "  ✓ Killed ScreenSaverEngine"
fi

sleep 1

echo ""
echo ">> Screensaver cache cleared - safe to test"
echo ""
echo "Next steps:"
echo "  1. Open System Settings"
echo "  2. Go to Screen Saver"
echo "  3. Select 'Boing Ball Saver'"
echo "  4. Test preview and fullscreen modes"
echo ""
echo "Check logs with:"
echo "  log stream --predicate 'subsystem == \"com.adamb3ll.BoingBallSaver\"' --level debug"
echo "  Or open Console.app and search for 'BoingBallSaver'"
echo ""
