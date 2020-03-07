# Ray-Tracer
Ray tracer using OpenGL's Compute Shader

Rendered scenes are static images (not real-time)

Loads vertex data from a .obj file (vertex data only)
Sends the loaded vertex data to a compute shader (CPU sends data to GPU)
Compute shader simulates rays emminating from the user's screen from each pixel (at angles such that parallax is simulated accurately)
These rays are tested for intersection with the object in the scene (represented by the vertex data)
Rays can be set to bounce 0 to 3 times (this creates levels of reflections)
Compute shader sends data back to CPU process which saves the data to an image
