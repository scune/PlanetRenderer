glslangValidator --target-env vulkan1.4 -V Shaders/PlanetShader.vert -o Shaders/Compiled/PlanetShader.vert.spv
glslangValidator --target-env vulkan1.4 -V Shaders/PlanetShader.frag -o Shaders/Compiled/PlanetShader.frag.spv

glslangValidator --target-env vulkan1.4 -V Shaders/PlanetShader.comp -o Shaders/Compiled/PlanetShader.comp.spv

# CBT
glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/SumReduction.comp -o Shaders/Compiled/Cbt/SumReduction.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/CachePointers.comp -o Shaders/Compiled/Cbt/CachePointers.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/IndirectDispatch.comp -o Shaders/Compiled/Cbt/IndirectDispatch.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/ResetCommands.comp -o Shaders/Compiled/Cbt/ResetCommands.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/UpdateBitfield.comp -o Shaders/Compiled/Cbt/UpdateBitfield.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/ReserveBlocks.comp  -o Shaders/Compiled/Cbt/ReserveBlocks.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/GenerateCmds.comp  -o Shaders/Compiled/Cbt/GenerateCmds.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/FillNewBlocks.comp  -o Shaders/Compiled/Cbt/FillNewBlocks.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/UpdateNeighbours.comp  -o Shaders/Compiled/Cbt/UpdateNeighbours.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/CacheVertices.comp  -o Shaders/Compiled/Cbt/CacheVertices.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/Classify.comp  -o Shaders/Compiled/Cbt/Classify.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/ValidateCmds.comp  -o Shaders/Compiled/Cbt/ValidateCmds.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/IndirectDispatchVertex.comp  -o Shaders/Compiled/Cbt/IndirectDispatchVertex.comp.spv

glslangValidator --target-env vulkan1.4 -V Shaders/Cbt/PlanetDisplacement.comp  -o Shaders/Compiled/Cbt/PlanetDisplacement.comp.spv
