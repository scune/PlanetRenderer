#!/bin/bash

export MESA_VK_TRACE=rgp
export AMD_PANEL_ENABLED=1
export MESA_VK_TRACE_TRIGGER=/tmp/rgp_capture
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.x86_64.json

SCRIPT_NAME=$(basename "$0")
echo "$SCRIPT_NAME: Environment set. Launching application..."
echo "$SCRIPT_NAME: Execute \"./touch /tmp/rgp_capture\" after launch in another console."

./testApp -x11
