#if __VERSION__ <= 310
precision mediump float;
#endif

#if 460 <= __VERSION__
    #define OPENGL_4_6
#endif

in vec3 v_fragPos;
in vec3 v_normal;

struct Light
{
    vec3 direction; // Light direction vector is determined from origin (0,0,0)
};

uniform vec3 u_color;
uniform Light u_light;
uniform vec3 u_viewPos;

layout(location = 0) out vec4 o_FragColor;

#if defined(OPENGL_4_6)
subroutine vec3 CreateDiffuse(vec3 norm, vec3 lightDir);
subroutine uniform CreateDiffuse createDiffuse;

subroutine vec3 CreateSpecular(vec3 norm, vec3 lightDir);
subroutine uniform CreateSpecular createSpecular;

subroutine ( CreateDiffuse )
vec3 DiffuseEnabled(vec3 norm, vec3 lightDir)
{
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_color;
    return diffuse;
}

subroutine ( CreateSpecular )
vec3 SpecularEnabled(vec3 norm, vec3 lightDir)
{
    vec3 viewDir = normalize(u_viewPos - v_fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    vec3 specular = 1.0 * spec * u_color;
    return specular;
}

subroutine ( CreateDiffuse, CreateSpecular )
vec3 Disabled(vec3 norm, vec3 lightDir)
{
    return vec3(0.0);
}
#else
struct AdsProperties
{
    int diffuseEnabled;
    int specularEnabled;
};
uniform AdsProperties u_adsProps;

vec3 createDiffuse(vec3 norm, vec3 lightDir)
{
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_color;
    return diffuse;
}

vec3 createSpecular(vec3 norm, vec3 lightDir)
{
    vec3 viewDir = normalize(u_viewPos - v_fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    vec3 specular = 1.0 * spec * u_color;
    return specular;
}
#endif

void main()
{
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * u_color;

    vec3 norm = normalize(v_normal);
    vec3 lightDir = normalize(-u_light.direction);

    #if defined(OPENGL_4_6)
    vec3 diffuse = createDiffuse(norm, lightDir);
    vec3 specular = createSpecular(norm, lightDir);
    #else
    vec3 diffuse = (u_adsProps.diffuseEnabled == 1) ? createDiffuse(norm, lightDir) : vec3(0.0);
    vec3 specular = (u_adsProps.specularEnabled == 1) ? createSpecular(norm, lightDir) : vec3(0.0);
    #endif

    vec3 result = ambient + diffuse + specular;
    o_FragColor = vec4(result, 1.0);
}
