


cbuffer ClothSimulation : register(b0)
{
    float3 gravity;
    float particle_mass;
    
    float particle_inverse_mass;
    float spring_k;
    float rest_length_horizontal;
    float rest_length_vertical;
    
    float rest_length_diagonal;
    float delta_t;
    float damping_constant;
    
    int n_particles_x;
    int n_particles_y;
    
    float3 wind_force;
}


struct Particle
{
    float3 position;
    float3 velocity;
};


StructuredBuffer<Particle> DataIn : register(t0);
RWStructuredBuffer<Particle> DataOut : register(u0);

[numthreads(1, 1, 1)]
void cs_main(uint3 DTid : SV_GroupID)
{
    uint idx = DTid.x * n_particles_y + DTid.y;
    
    float3 p = DataIn[idx].position;
    float3 v = DataIn[idx].velocity;
    float3 r = float3(0, 0, 0);
    
    float3 force = gravity * particle_mass;
    
    //down 
    if (DTid.y > 0)
    {
        r = DataIn[idx - 1].position - p;
        force += normalize(r) * spring_k * (length(r) - rest_length_vertical);
        
        //down right
        if (DTid.x < n_particles_x - 1)
        {
            r = DataIn[idx - 1 + n_particles_y].position - p;
            force += normalize(r) * spring_k * (length(r) - rest_length_diagonal);
        }
        //down left
        if (DTid.x > 0)
        {
            r = DataIn[idx - 1 - n_particles_y].position - p;
            force += normalize(r) * spring_k * (length(r) - rest_length_diagonal);
        }
    }
    
    //up
    if (DTid.y < n_particles_y - 1)
    {
        r = DataIn[idx + 1].position - p;
        force += normalize(r) * spring_k * (length(r) - rest_length_vertical);
        
        //up right
        if (DTid.x < n_particles_x - 1)
        {
            r = DataIn[idx + 1 + n_particles_y].position - p;
            force += normalize(r) * spring_k * (length(r) - rest_length_diagonal);
        }
        //up left
        if (DTid.x > 0)
        {
            r = DataIn[idx + 1 - n_particles_y].position - p;
            force += normalize(r) * spring_k * (length(r) - rest_length_diagonal);
        }
    }
    
    //right
    if (DTid.x < n_particles_x - 1)
    {
        r = DataIn[idx + n_particles_y].position - p;
        force += normalize(r) * spring_k * (length(r) - rest_length_horizontal);
    }
    
        //left
    if (DTid.x > 0)
    {
        r = DataIn[idx - n_particles_y].position - p;
        force += normalize(r) * spring_k * (length(r) - rest_length_horizontal);
    }

    force -= damping_constant * v;
    
    force += wind_force;
    
    
    //if ((DTid.y == n_particles_y - 1 || DTid.y == 0) && (DTid.x == 0 || DTid.x == n_particles_x - 1))
    if (DTid.y == n_particles_y - 1)// && (DTid.x == 0 || DTid.x == n_particles_x - 1 || DTid.x == n_particles_x / 2))
    {
        DataOut[idx].position = p;
        
        DataOut[idx].velocity = float3(0, 0, 0);

    }
    else
    {
        float3 a = force * particle_inverse_mass;
#ifdef VERLET
        Particle old = DataOut[idx];
        DataOut[idx].velocity = DataIn[idx].position - old.position;
        DataOut[idx].position = 2 * DataIn[idx].position - old.position + delta_t * delta_t * a;
#else
       
       float new_velocity = float3(v + a * delta_t);
       
       DataOut[idx].velocity = new_velocity;
       
       DataOut[idx].position = float3(p + new_velocity * delta_t); //+0.5 * delta_t * delta_t * a);
 #endif
        

    }

}