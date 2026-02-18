#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

// ── material ────────────────────────────────────────────────────────────────
struct Material {
    vec3  color;
    float specular;
    float shininess;
};
uniform Material material;

// ── directional light (fill / ambient) ──────────────────────────────────────
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform DirLight dirLight;

// ── point lights (floating orbs) ────────────────────────────────────────────
#define MAX_LIGHTS 8
struct PointLight {
    vec3  position;
    vec3  color;
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight lights[MAX_LIGHTS];
uniform int numLights;

uniform vec3 viewPos;

// ── helpers ─────────────────────────────────────────────────────────────────
vec3 calcDirLight(DirLight light, vec3 norm, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    // specular  (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), material.shininess);

    vec3 ambient  = light.ambient  * material.color;
    vec3 diffuse  = light.diffuse  * diff * material.color;
    vec3 specular = light.specular * spec * material.specular;
    return ambient + diffuse + specular;
}

vec3 calcPointLight(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir)
{
    vec3  lightDir  = normalize(light.position - fragPos);
    float diff      = max(dot(norm, lightDir), 0.0);
    vec3  halfDir   = normalize(lightDir + viewDir);
    float spec      = pow(max(dot(norm, halfDir), 0.0), material.shininess);
    float dist      = length(light.position - fragPos);
    float atten     = 1.0 / (light.constant + light.linear * dist
                             + light.quadratic * dist * dist);

    vec3 diffuse  = light.color * diff * material.color  * atten;
    vec3 specular = light.color * spec * material.specular * atten;
    return diffuse + specular;
}

// ── main ────────────────────────────────────────────────────────────────────
void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // directional fill
    vec3 result = calcDirLight(dirLight, norm, viewDir);

    // point lights
    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++)
        result += calcPointLight(lights[i], norm, FragPos, viewDir);

    // gentle rim light for depth
    float rim = 1.0 - max(dot(norm, viewDir), 0.0);
    rim = pow(rim, 3.0) * 0.15;
    result += rim * vec3(0.5, 0.7, 1.0);

    FragColor = vec4(result, 1.0);
}
