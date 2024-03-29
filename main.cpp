#include <SDL2/SDL.h>
#include <cmath>
#include <vector>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

#define WINDOW_W 800
#define WINDOW_H 600

struct vector3d
{
    double x;
    double y;
    double z;

    vector3d()
    {
        x = 0.0; y = 0.0; z = 0.0;
    }

    vector3d operator +(vector3d const& vec)
    {
        vector3d out;
        out.x = this->x + vec.x;
        out.y = this->y + vec.y;
        out.z = this->z + vec.z;
        return out;
    }

    vector3d operator -(vector3d const& vec)
    {
        vector3d out;
        out.x = this->x - vec.x;
        out.y = this->y - vec.y;
        out.z = this->z - vec.z;
        return out;
    }

    // scaling
    vector3d operator *(const double scalar)
    {
        vector3d out;
        out.x = this->x * scalar;
        out.y = this->y * scalar;
        out.z = this->z * scalar;
        return out;
    }

    vector3d operator /(const double scalar)
    {
        vector3d out;
        out.x = this->x / scalar;
        out.y = this->y / scalar;
        out.z = this->z / scalar;
        return out;
    }

    double operator *(vector3d const& vec)
    {
        double out = this->x * vec.x + this->y * vec.y + this->z * vec.z;
        return out;
    }

    double mag()
    {
        double out = sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
        return out;
    }

    // normal mathematical: counter-clockwise is positive
    vector3d rotate(double theta)
    {
        vector3d out;
        out.x = this->x * cos(theta) - this->y * sin(theta);
        out.y = this->x * sin(theta) + this->y * cos(theta);
        out.z = this->z;
        return out;
    }
};

struct entity
{
    vector3d pos;
    vector3d vel;
    vector3d acc;

    void act(const double dt)
    {
        pos = pos + vel * dt;
        vel = vel + acc * dt;
    }
};

struct viewport
{
    entity e;
    int w;
    int h;

    viewport(int w, int h)
    {
        this->w = w;
        this->h = h;
    }
};

enum control
{
    CTRL_NONE,
    CTRL_FORWARD,
    CTRL_BACKWARD,
    CTRL_TURN_L,
    CTRL_TURN_R,
    CTRL_FIRE
};

// converts global coordinates to SDL surface coordinates w/ viewport entity
// no viewport rotation supported yet
void cart2sdl(const viewport &vp, vector3d pos, int *x, int *y)
{
    // relative position of entity to viewport center
    vector3d corrected = pos - vp.e.pos;
    *x= (int)(corrected.x + ((double) vp.w / 2));
    *y= (int)(((double)vp.h / 2) - corrected.y);
}

void render_clear(SDL_Renderer *renderer)
{
    SDL_RenderClear(renderer);
}

void render_background(SDL_Renderer *renderer, const viewport &vp)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect r = {0, 0, vp.w, vp.h};
    SDL_RenderFillRect(renderer, &r);
}

void render_player(SDL_Renderer *renderer, const viewport &vp, vector3d pos)
{
    // triangle points
    vector3d t; t.x =  5.0; t.y =  0.0;
    vector3d b; b.x = -5.0; b.y =  0.0;
    vector3d l; l.x = -5.0; l.y =  5.0;
    vector3d r; r.x = -5.0; r.y = -5.0;
    t = pos + t.rotate(pos.z);
    b = pos + b.rotate(pos.z);
    l = pos + l.rotate(pos.z);
    r = pos + r.rotate(pos.z);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Point points[5];
    cart2sdl(vp, t, &points[0].x, &points[0].y);
    cart2sdl(vp, l, &points[1].x, &points[1].y);
    cart2sdl(vp, r, &points[2].x, &points[2].y);
    points[3] = points[0];
    cart2sdl(vp, b, &points[4].x, &points[4].y);
    SDL_RenderDrawLines(renderer, points, 5);
}

void render_projectile(SDL_Renderer *renderer, const viewport &vp, vector3d pos)
{
    vector3d t; t.x =  5.0; t.y =  0.0;
    vector3d b; t.x = -5.0; t.y =  0.0;
    t = pos + t.rotate(pos.z);
    b = pos + b.rotate(pos.z);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Point points[2];
    cart2sdl(vp, t, &points[0].x, &points[0].y);
    cart2sdl(vp, b, &points[1].x, &points[1].y);
    SDL_RenderDrawLines(renderer, points, 2);
}

struct projectile
{
    entity e;
    double d;

    projectile()
    {
        this->d = 0.0;
    }

    void act(double dt)
    {
        vector3d vel = e.vel;
        vel.z = 0.0;
        vel = vel * dt;
        this->d = this->d + vel.mag();
        e.act(dt);
    }
};

struct renderable
{
    enum type
    {
        PLAYER,
        PROJECTILE
    };
    const entity *e;
    type t;

    renderable(const entity *e, type t)
    {
        this->e = e;
        this->t = t;
    }
};

void render_flip(SDL_Renderer *renderer)
{
    SDL_RenderPresent(renderer);
}

void render(SDL_Renderer *renderer, const viewport &vp, std::vector<renderable> *render_lists, int num_render_lists)
{
    render_clear(renderer);
    render_background(renderer, vp);
    for (int i = 0; i < num_render_lists; ++i) {
        for (renderable &r : render_lists[i]) {
            if (r.t == renderable::type::PLAYER)
                render_player(renderer, vp, r.e->pos);
            else if (r.t == renderable::type::PROJECTILE)
                render_projectile(renderer, vp, r.e->pos);
        }
    }
    render_flip(renderer);
}

control key2ctrl(void)
{
    const uint8_t *keyStates = SDL_GetKeyboardState(NULL);

    if (keyStates[SDL_SCANCODE_W])          return CTRL_FORWARD;
    else if (keyStates[SDL_SCANCODE_S])     return CTRL_BACKWARD;
    else if (keyStates[SDL_SCANCODE_A])     return CTRL_TURN_L;
    else if (keyStates[SDL_SCANCODE_D])     return CTRL_TURN_R;
    else if (keyStates[SDL_SCANCODE_SPACE]) return CTRL_FIRE;
    else                                    return CTRL_NONE;
}

double get_time(void)
{
    double t = ((double)SDL_GetTicks()) / 1000.0;
    return t;
}

#define FIRE_VEL 100.0
void spawn_projectile(entity *src, std::vector<projectile> *proj_list, std::vector<renderable> *render_list)
{
    vector3d add_vel;
    add_vel.x = 100.0 * cos(src->pos.z);
    add_vel.y = 100.0 * sin(src->pos.z);

    projectile proj;
    proj.e = *src;
    proj.e.vel.z = 0.0; // don't keep turning vel
    proj.e.acc = vector3d(); // don't keep acceleration
    proj.e.vel = proj.e.vel + add_vel;
    proj_list->push_back(proj);
    render_list->push_back(renderable(&(proj_list->back().e), renderable::type::PROJECTILE));
}

#define F_ACC_MAG 100.0
#define B_ACC_MAG 100.0
#define T_ACC_MAG 10.0
void handle_controls(entity *e, control c)
{
    e->acc = vector3d();
    switch (c)
    {
    case CTRL_FORWARD:
        e->acc.x = F_ACC_MAG * cos(e->pos.z);
        e->acc.y = F_ACC_MAG * sin(e->pos.z);
        break;
    case CTRL_BACKWARD:
        e->acc.x = -B_ACC_MAG * cos(e->pos.z);
        e->acc.y = -B_ACC_MAG * sin(e->pos.z);
        break;
    case CTRL_TURN_L:
        e->acc.z = +T_ACC_MAG;
        break;
    case CTRL_TURN_R:
        e->acc.z = -T_ACC_MAG;
        break;
    default:
        break;
    }
}

#define X_VEL_CAP 400.0
#define Y_VEL_CAP 400.0
#define Z_VEL_CAP 5.0
void cap_vel(entity *e)
{
    if      (e->vel.x >  X_VEL_CAP) e->vel.x =  X_VEL_CAP;
    else if (e->vel.x < -X_VEL_CAP) e->vel.x = -X_VEL_CAP;
    else if (e->vel.y >  Y_VEL_CAP) e->vel.y =  Y_VEL_CAP;
    else if (e->vel.y < -Y_VEL_CAP) e->vel.y = -Y_VEL_CAP;
    else if (e->vel.z >  Z_VEL_CAP) e->vel.z =  Z_VEL_CAP;
    else if (e->vel.z < -Z_VEL_CAP) e->vel.z = -Z_VEL_CAP;
}

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0 ) {
        fprintf(stderr,"Error: could not initialized SDL\n");
        return 1;
    }

    window = SDL_CreateWindow("ts", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    //renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    // test render
    viewport vp(WINDOW_W, WINDOW_H);
    entity  player;
    control player_ctrl = CTRL_NONE;
    std::vector<projectile> proj_list;
    std::vector<renderable> render_lists[2];
    std::vector<renderable> *ship_render_list = &render_lists[0];
    std::vector<renderable> *proj_render_list = &render_lists[1];

    ship_render_list->push_back(renderable(&player, renderable::type::PLAYER));
    render(renderer, vp, render_lists, 2);

    SDL_Event event;
    double t = get_time();
    double t_new;
    double t_fire = 0.0;
    double dt;
    while(1) {
        // event loop

        // get dt, update time
        t_new  = get_time();
        dt = t_new - t;
        t = t_new;

        // check for events
        if (SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_QUIT:
                goto cleanup;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                player_ctrl = key2ctrl();
                break;
            }
        }

        // handle fire timer
        if (player_ctrl == CTRL_FIRE) {
            if (t - t_fire < 1.0) {
                player_ctrl = CTRL_NONE;
            } else {
                t_fire = t;
            }
        }

        // process controls
        handle_controls(&player, player_ctrl);
        if (player_ctrl == CTRL_FIRE) {
            spawn_projectile(&player, &proj_list, proj_render_list);
        }

        // act all entities
        player.act(dt);
        cap_vel(&player);
        // todo optimization: reverse iterator
        auto proj_it = proj_list.begin();
        auto proj_render_it = proj_render_list->begin();
        while (proj_it != proj_list.end()) {
            proj_it->act(dt);
            if (proj_it->d > 400.0) {
                proj_it = proj_list.erase(proj_it);
                proj_render_it = proj_render_list->erase(proj_render_it);
            } else {
                // only inc on else
                proj_it++;
                proj_render_it++;
            }
        }
        //printf("%0.2f: %0.2f, %0.2f, %0.2f\n", dt, player.vel.x, player.vel.y, player.vel.z);


        // render final results
        render(renderer, vp, render_lists, 2);
        SDL_Delay(20);
    }

cleanup:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return 0;
}
