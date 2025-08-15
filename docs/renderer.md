
### vulkan features/extensions
-	dynamic rendering, descriptor indexing, buffer device address, mesh shaders
	shader objects, acceleration structures

### notes:
-	bindless descriptors for accessing
	textures/storage images/combined image samplers/accel structures,
	all in one global descriptor set
-	buffer device address for accessing uniform and storage buffers
-	visibility buffer shading, 
-	gpu based, meshlet geometry processing using mesh shaders

### render graph
-	references:
		- https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/
			- NOTE: flush = make available, invalidate = make visible
		- https://poniesandlight.co.uk/reflect/island_rendergraph_1/
		- https://tadriansen.dev/2025-04-21-building-a-vulkan-render-graph/
-	manages synchronisation and scheduling of rendering
-	each node is a render pass and the edges are formed from
	read/write dependencies
-	each pass stores:
		- type (GRAPHICS, COMPUTE, TRANSFER)
		- dependencies
		- all inputs/outputs and accessed resources, specifying for each:
			- access type (READ_ONLY, WRITE_ONLY, READ_WRITE)
			- resource type (STORAGE IMAGE, BUFFER, TEXTURE, etc.)
		- shader object (not required for all passes)
		- callback to record its draw commands
-	passes only get executed if they contribute to the final frame
-	passes can be marked as root passes, meaning they always get run,
	e.g. final blit from main draw img to swapchain img
-	manage resources logically rather than physically, e.g. naming inputs/outputs
-	building/baking
		- render graph must be flattened from a 
		  graph of passes, into a list of passes in order to be executed
		- validate:
			- ensure all inputs and outputs match
		- traverse dependency graph:
			- directed acyclic graph of passes that must be
			  flattened into list of passes
			- traverse tree bottom up
		- pass reordering:
			- aim for optimal overlap between passes, minimise
			  hard barrierss
			- e.g. if pass A writes data and pass B reads it,
			  maximise the number of passes between A and B
		- logical to physical resource assignment:
			- assign physical resource id to all resources
			- alias resources which do read-modify-write
		- building barriers:
			- each resource goes through three stages per pass:
				- transition to appropriate layout, cache
				  must be invalidated
				- resource is used (reads/writes)
				- resource finishes in new layout, with
				  potential writes needing to be flushed
				  later
			- for each pass, build list of invalidates and flushes
				- inputs are put in invalidate bucket
				- outputs are put in flush bucket
				- read-modify-write resources get entry in both

		- target aliasing/temporally alias resources
			- reuse physical resources that are used at
			  different times across the frame, rather than
			  needlessly allocated a seperate resource
			- method: for each resource find the first and last
			  pass where its used. if another resource exists with
			  the same format/dimensions, and their pass range
			  does not overlap, then alias them
			- resources can be marked as never alias, e.g. cases
			  where a last frame and current frame copy exists
			- aliasing might not be optimal in all cases as it
			  may increase the number of barriers that exist