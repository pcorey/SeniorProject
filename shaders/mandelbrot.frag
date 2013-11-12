varying vec3 fragPos;
uniform sampler2D colorTex;

uniform float zoomFactor=1.0;
uniform vec2 zoomTarget;

int m=8,n=24,t=16777216;

int floatToFP(float a) {
	return a*t;
}

float FPToFloat(int a) {
	return a/t;
}

vec2 complexMult(vec2 a,vec2 b) {
    vec2 res;
    res.x=a.x*b.x-a.y*b.y;
    res.y=a.y*b.x+a.x*b.y;
    return res;
}

float complexAbs(vec2 a) {
    return sqrt(a.x*a.x+a.y*a.y);
}

void main()
{
    vec2 z=vec2(0,0);
    vec2 c=zoomTarget.xy+fragPos.xy*zoomFactor;
    bool inSet=true;
    
    int maxIter=2000;
    int escapeVal=0;

    for (int i=0;(i<maxIter)&&(inSet);i++) {
        z=complexMult(z,z)+c;
        if (complexAbs(z)>=2.0) {
            inSet=false;
            escapeVal=i;
        }
    }
    if (inSet) {
        gl_FragColor=vec4(1,1,1,1);
    }
    else { 
        //gl_FragColor=vec4(1.0*escapeVal/maxIter,1.0*escapeVal/maxIter,1.0*escapeVal/maxIter,1);
        vec4 color = texture2D(colorTex,vec2(0,(float)escapeVal/maxIter));
        gl_FragColor=color;
    }
        
        
}
