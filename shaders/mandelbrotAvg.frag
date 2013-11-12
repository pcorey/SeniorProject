//triangle ineqaulity average algorithm based on algorithm found in UltraFractal

varying vec3 fragPos;
uniform sampler2D colorTex;

uniform float zoomFactor=1.0;
uniform vec2 zoomTarget;

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
    
    int maxIter=2000;
    float bailout=8000000.0;
    bool inSet=true;
    
    float sum=0;
    float sum2=0;
    float ac=complexAbs(c);
    float il=1.0/log(2.0);
    float lp=log(log(bailout)/2.0);
    float az2=0.0;
    float lowbound=0.0;
    float f=0.0;
    float index=0.0;
    
    int escapeVal=0;

    vec2 zOld;
    for (int i=0;i<maxIter;i++) {
        zOld=z;
        z=complexMult(z,z)+c;
        if (complexAbs(z)>=bailout) {
            escapeVal=i;
            inSet=false;
            z=zOld;
            break;
        }
        sum2=sum;
        if ((i!=0)&&(i!=maxIter-1)) {
            az2=complexAbs(z-c);
            lowbound=abs(az2-ac);
            sum+=((complexAbs(z)-lowbound)/(az2+ac-lowbound));
        }
    }
    sum=sum/escapeVal;
    sum2=sum2/(escapeVal-1.0);
    float cabs=complexAbs(z);
    f=il*lp - il*log(log(cabs));
    index=sum2+(sum-sum2)*(f+1.0);
    vec4 color;
    if (!inSet)
        color = texture2D(colorTex,vec2(0,index));
    else
        color=vec4(1,1,1,1);
    gl_FragColor=color;
}
