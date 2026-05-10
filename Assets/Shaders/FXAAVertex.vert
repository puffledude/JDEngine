#version 460 core


layout (location =0) in vec3 position;
layout (location =1) in vec3 colour;
layout (location =2) in vec2 texCoord;


layout (location =0) out Output{
	vec2 texCoord;

}vert;



void main(void){
	vert.texCoord = texCoord;
	gl_Position = vec4(position, 1.0);
}