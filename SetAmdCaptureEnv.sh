#!/bin/bash

export MESA_VK_TRACE=rgp
export AMD_PANEL_ENABLED=1
export MESA_VK_TRACE_TRIGGER=/tmp/rgp_capture
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/radeon_icd.x86_64.json
echo "Enviroment set, now open app using x11 and type \"touch /tmp/rgp_capture\" to save captures in \"/tmp/\""
