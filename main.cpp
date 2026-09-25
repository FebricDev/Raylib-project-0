#include <raylib.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <memory>
#include <math.h>
#include "main.h"

const int screenWidth = 900;
const int screenHeight = 800;

#define AMOUNT_OF_ENEMIES 2

#define JRAVITY_STRENGTH 10.0f
#define MAX_VELOCITY 100
#define MAX_FRICTION_VALUE 1.0f
#define MIN_FRICTION_VALUE 0.0f
#define BULLET_SPEED 500.0f
#define BULLET_MAX_LIFE_TIME 1.0f
#define STANDART_PARTICLE_LIFE_TIME 1.0f

#define DrawSpeedomeeter
#define DrawHitBoxes
#define DrawInvinsibleBar
#define DrawSprites
#define BlockMouseInWindow
#define ShowCursor
#define DynamicCursorRotation
#define DrawSelector
#define DynamicEnemySelector
#define AdditionalSelectorOffsetFromCenterToMouse
//#define ArrowShooting

#ifdef DrawSelector
	#define SelectorAccuracy 3.5f
	#define SelectorOffset 1.5f

#endif

#define TO_RAD(a) a * PI / 180.0f

Sound hit_sound;

void PlayHitSound()
{
	float random_pitch = float(GetRandomValue(-1, 1)) * 0.05f;
	SetSoundPitch(hit_sound, 1.0f + random_pitch);
	PlaySound(hit_sound);
}

Vector2 GetNormByAngle(float angle, float angle_rand_deg = 0)
{
	int rand_angle_distirbance = GetRandomValue(-angle_rand_deg * 100.0f / 2.0f , angle_rand_deg * 100.0f / 2.0f);
	angle = TO_RAD(angle + rand_angle_distirbance / 100.0f);
	return Vector2{ (float)cos(angle), (float)sin(angle)};
}

Vector2 NormalizeVec(Vector2 vec)
{
	float length = sqrt(vec.x * vec.x + vec.y * vec.y);
	return { vec.x / length, vec.y / length };
}

float clamp(float min, float max, float value)
{
	return std::max(std::min(value, max), min);
}

bool AreTouching(Vector2 pos1, float radius1, Vector2 pos2, float radius2)
{
	Vector2 d;
	d.x = pos1.x - pos2.x;
	d.y = pos1.y - pos2.y;
	float a = hypot(d.x, d.y);
	return a < radius1 + radius2;
}

class Entity
{
public:

	Entity(float x, float y, float radius) :
		position({ x, y }), radius(radius) { }

	Vector2 GetPosition()
	{
		return position;
	}

	float GetRadius()
	{
		return radius;
	}

protected:
	Vector2 position;
	float radius;
};

class Health
{
public:

	enum HealType
	{
		hp50,
		hp100,
		hp250
	};
	enum DamageType
	{
		dmg_enemy,
		dmg_bullet,
		dmg_
	};

	Health(float max_amount) : max_amount(max_amount), current_amount(max_amount) {}

	void Heal(int heal_type)
	{
		switch (heal_type)
		{
		case HealType::hp50:
			IncreaseHp(50);
			break;
		case HealType::hp100:
			IncreaseHp(100);
			break;
		case HealType::hp250:
			IncreaseHp(250);
			break;
		default:
			printf("[ ERROR ]: Unknown healing type\n");
		}
	}
	void Damage(int damage_type)
	{
		printf("Damaging\n");
		switch (damage_type)
		{
		case DamageType::dmg_enemy:
			DecreaseHp(50);
			break;
		case DamageType::dmg_bullet:
			DecreaseHp(20);
			break;
		case DamageType::dmg_:
			DecreaseHp(100);
			break;
		default:
			printf("[ ERROR ]: Unknown damage type\n");
		}
	}
	float Get() { return this->current_amount; }
	float GetMaxHP() { return this->max_amount; }

private:
	void IncreaseHp(float amount)
	{
		if (current_amount + amount > max_amount)
			current_amount = max_amount;
		current_amount += max_amount;
	}
	void DecreaseHp(float amount)
	{
		if (current_amount - amount < 0)
			current_amount = 0;
		current_amount -= amount;
	}

	float max_amount;
	float current_amount;
};

class Mob : public Entity
{
public:

	enum class HitFlags : uint8_t
	{
		HITTED_BY_ENEMY = 0b0001,
		HITTED_BY_PLAYER = 0b0010,
		HITTED_BY_BULLET = 0b0100,

		MODIFIER_BULLET_HURT_MORE = 0b1000000
	};

	enum Type
	{
		Enemy,
		Player
	};

	Mob(Type type, float x, float y, float radius, int hp) : Entity(x, y, radius), hp(hp)
	{
		this->type = type;
		velocity = { 0,0 };
		wait_for = 1.1;
		hit_timer = 0;
	}

	virtual void PlayHitAnimation() = 0;

	Type GetType()
	{
		return this->type;
	}

	void Tick_Update()
	{
		hit_timer += GetFrameTime();

	}

	bool CanGetHurt()
	{
		return hit_timer > wait_for;
	}

	void Hit(float dx, float dy, uint8_t hit_flags)
	{
		if (!(hit_flags & (uint8_t)Mob::HitFlags::HITTED_BY_BULLET))
		{
			if (!CanGetHurt())
				return;
		}
		//else
			//printf("bullet hit\n");
		hit_timer = 0;

		velocity.x += dx;
		velocity.y += dy;

		PlayHitSound();
		
		if ((hit_flags & (uint8_t)HitFlags::HITTED_BY_ENEMY) && type == Mob::Player) // Player got hit by enemy
		{
			printf("Enemy hitted player. Hit flags: [%d]\n", hit_flags);
			if (hit_flags & (uint8_t)Mob::HitFlags::HITTED_BY_BULLET)
				hp.Damage(Health::DamageType::dmg_bullet);
		}
		else if ((hit_flags & (uint8_t)HitFlags::HITTED_BY_PLAYER) && type == Mob::Enemy) // Enemy got hit by player
		{
			printf("Player hitted enemy. Hit flags: [%d]\n", hit_flags);
			if (hit_flags & (uint8_t)Mob::HitFlags::HITTED_BY_BULLET)
				hp.Damage(Health::DamageType::dmg_bullet);
			if (hit_flags & (uint8_t)Mob::HitFlags::MODIFIER_BULLET_HURT_MORE)
				hp.Damage(Health::DamageType::dmg_bullet);
		}
		else
		{
			printf("Unknown hit flag: [%d]\n", hit_flags);
			hp.Damage(Health::DamageType::dmg_enemy);
		}

		//if (hit_flags &= (uint8_t)HitFlags::HITTED_BY_PLAYER)
		//	hp.Damage(Health::DamageType::dmg_);
	}

	bool IsAlive()
	{
		return (hp.Get() > 0);
	}

	void DrawBars()
	{
		float width = GetRadius() * 2;
		float k = hp.Get() / hp.GetMaxHP();
		// HealthBar BG
		DrawRectangle(GetPosition().x - GetRadius(), GetPosition().y + GetRadius() + 10, width, 5, BLACK);
		// HealthBar itself
		DrawRectangle(GetPosition().x - GetRadius(), GetPosition().y + GetRadius() + 10, width * k, 5, GREEN);

#ifdef DrawInvinsibleBar
		float _hit_timer = hit_timer;
		if (_hit_timer > wait_for)
			_hit_timer = wait_for;
		float kk = _hit_timer / wait_for;
		// InvinsibleBar BG
		DrawRectangle(GetPosition().x - GetRadius(), GetPosition().y + GetRadius() + 10 + 10, width, 5, BLACK);
		// InvinsibleBar itself														    
		DrawRectangle(GetPosition().x - GetRadius(), GetPosition().y + GetRadius() + 10 + 10, width * kk, 5, Color{0,255,255,255});
#endif
	}

protected:
	Type type;
	Vector2 velocity;
	Health hp;
	float hit_timer;
	float wait_for;
};

class Bullet
{
public:

	enum BulletType
	{
		Enemy,
		Player,
		NPC
	};
	enum class BulletClass
	{
		OneHit
	};

	Bullet(BulletType bul_type, Vector2 pos, Vector2 vel, float live_for)
	{
		this->live_for = live_for;
		prev_position = pos;
		position = pos;
		velocity = vel;
		color = RED;
		color.a = 100;
		radius = 10;

		friction = MIN_FRICTION_VALUE; // max
		life_time = 0;
		bullet_class = BulletClass::OneHit;
		bullet_type = bul_type;
		hitted_amount = 0;
	}

	Vector2 GetPosition() { return this->position; }
	Vector2 GetVelocity() { return this->velocity; }
	float GetRadius() { return this->radius; }
	BulletClass GetBulletClass() { return this->bullet_class; }
	BulletType GetBulletType() { return this->bullet_type; }

	void Update()
	{
		float delta_time = GetFrameTime();

		life_time += delta_time;

		prev_position = position;

		position.x += velocity.x * delta_time;
		position.y += velocity.y * delta_time;

		velocity.x *= (MAX_FRICTION_VALUE - friction);
		velocity.y *= (MAX_FRICTION_VALUE - friction);
	}
	void Draw()
	{
		//printf("%.02f %.02f\n", position.x, position.y);
		if (bullet_type == BulletType::Enemy)
			color = RED;
		else if (bullet_type == BulletType::Player)
			color = YELLOW;
		else
			color = BLUE;
		DrawRing(position, radius, radius + 5, 0, 360, 0, color);
	}

	void IncrimentHitted() { hitted_amount++; }
	int GetHittedAmount() { return hitted_amount; }
	bool IsAlive()
	{
		return (life_time < live_for);
	}


private:
	Vector2 prev_position;
	Vector2 position;
	Vector2 velocity;
	Color color;
	float friction;

	float life_time;
	float live_for;

	float radius;
	int hitted_amount;
	BulletClass bullet_class;
	BulletType bullet_type;
};

class BulletHandler
{
public:
	void Update()
	{
		//if(bullets.size())
			//printf("Updating...\n");
		for (int i = 0; i < bullets.size(); i++)
		{
			//printf("%d, ", i);
			bullets[i].Update();

			bool must_delete = false;
			switch (bullets[i].GetBulletClass())
			{
			case Bullet::BulletClass::OneHit:
				if(bullets[i].GetHittedAmount() > 0)
					must_delete = true;
				break;
			default:
				break;
			}

			if (!bullets[i].IsAlive() || must_delete)
			{
				printf("[bullet deleted]\n");
				must_delete = false;
				bullets.erase(std::begin(bullets) + i);
				i--;
				
			}
		}
		//if(bullets.size())
			//printf("\n");
	}
	void PushBullet(Bullet bullet)
	{
		bullets.push_back(bullet);
	}
	Bullet& GetBullet(int index) { if (index >= 0 && index < bullets.size()) return bullets[index]; }
	unsigned int GetBulletAmount() { return (unsigned int)bullets.size(); }
	void Draw()
	{
		for (int i = 0; i < bullets.size(); i++)
		{
			bullets[i].Draw();
		}
	}
private:
	std::vector<Bullet> bullets;
};

class Particle
{
public:
	enum ParticleDraw
	{
		NONE_OR_COLOR,
		Texture
	};

	Particle(Vector2 position = {0,0}, Vector2 velocity = {0,0}, ParticleDraw particle_draw = ParticleDraw::NONE_OR_COLOR, float live_for = STANDART_PARTICLE_LIFE_TIME) :
		position(position), velocity(velocity)
	{
		friction = MIN_FRICTION_VALUE;
		this->live_for = live_for;
		life_timer = 0;
		original_color = MAGENTA;
		color_to_change_to = original_color;
		size = { 10,10 };

		scale_change_function_1 = NULL; // example: sqrt
		scale_change_function_2 = NULL; // example: pow

		this->particle_draw = particle_draw;
	}

	void SetColor(Color color) { original_color = color; }
	void SetColorToChangeTo(Color color) { color_to_change_to = color; }
	void SetScaleChangeFunc1(double (*func)(double)) { scale_change_function_1= func; }
	void SetScaleChangeFunc2(double (*func)(double, double)) { scale_change_function_2 = func; }
	void AddTexture(const char* texture_path)
	{
		texture_vector.emplace_back(LoadTexture(texture_path));
	}

	void SetPosition(Vector2 _position) { 

		//printf("setting particle new pos to x=%.02f, y=%.02f\n", _position.x, _position.y);
		this->position = _position; 
	}
	void SetVelocity(Vector2 _velocity) { this->velocity = _velocity; }

	bool IsAlive() { return life_timer <= live_for; }

	void ApplyVelocityToPosition()
	{
		float delta_time = GetFrameTime();
		position.x += velocity.x * delta_time;
		position.y += velocity.y * delta_time;

		velocity.x *= (MAX_FRICTION_VALUE - friction);
		velocity.y *= (MAX_FRICTION_VALUE - friction);
		
		//printf("particle pos: x=%.02f, y=%.02f\n", position.x, position.y);

	}

	void UpdateParams()
	{
		float delta_time = GetFrameTime();
		life_timer += delta_time;
		float x = life_timer / live_for;
		if (scale_change_function_1 != NULL)
		{
			current_scale = scale_change_function_1(1 - x);
		}
		else if (scale_change_function_2 != NULL)
		{
			const float second_param = 2;
			current_scale = scale_change_function_2(1 - x, second_param);
		}
	}

	void Draw()
	{
		Color color = original_color;
		if (particle_draw == ParticleDraw::Texture && texture_vector.size())
		{
			
			int texture_index = (life_timer / live_for) * (texture_vector.size()); // (vec size = 4) 0..1 * 3 = 0..3
			Texture2D current_texture = texture_vector[texture_index];

			Rectangle dest_rect = {
				position.x,
				position.y,
				(float)current_texture.width * pixel_scale,
				(float)current_texture.height * pixel_scale
			};

			DrawTexturePro(current_texture, Rectangle{ 0.0f, 0.0f, (float)current_texture.width, (float)current_texture.height }, dest_rect, {0,0}, 0.0f, WHITE);
		}
		else
		{
			DrawRectangle(position.x - size.x / 2, position.y - size.y / 2, size.x, size.y, color);
		}
	}

private:
	Vector2 position;
	Vector2 velocity;
	Vector2 size;
	float friction;
	float live_for;
	float life_timer;

	double(*scale_change_function_1)(double); // function
	double(*scale_change_function_2)(double, double); // function
	Color original_color;
	Color color_to_change_to; // to not change must be equal to original_color

	ParticleDraw particle_draw;
	float current_scale;
	std::vector<Texture2D> texture_vector;
};

class ParticleEmiter
{
public:
	enum EmiterAddition
	{
		Rand_Velocity,
		Rand_Life_Timer
	};


	ParticleEmiter(int amount, uint8_t particles_flags) : 
		particles(amount)
	{}

	void PushParticle(Particle& particle) { particles.emplace_back(particle); }

	void SetParticle(int index, Particle& particle)
	{
		if (index < 0 && index >= particles.size()) return;

		particles[index] = particle;
	}
	size_t GetSize() { return particles.size(); }

	void SetPositionToEmitFrom(Vector2 position_to_emit_from)
	{
		//printf("setting particles pos to x=%.02f, y=%.02f\n", position_to_emit_from.x, position_to_emit_from.y);
		for (int i = 0; i < particles.size(); i++)
		{
			//printf("[%d] ", i);
			particles[i].SetPosition(position_to_emit_from);
		}
	}

	void SetVelocityToEmitWith(Vector2 velocity)
	{
		for (int i = 0; i < particles.size(); i++)
		{
			float angle = atan2(velocity.y, velocity.x);
			Vector2 rand_vel = GetNormByAngle(angle, 360);
			rand_vel.x *= velocity.x;
			rand_vel.y *= velocity.y;
			//particles[i].SetVelocity({ velocity.x + rand_vel.x, velocity.y + rand_vel.y });
			particles[i].SetVelocity(rand_vel);
		}
	}

	void Update()
	{
		float delta_time = GetFrameTime();
		for (int i = 0; i < particles.size(); i++)
		{
			particles[i].ApplyVelocityToPosition();
			particles[i].UpdateParams();

			if (!particles[i].IsAlive())
			{
				particles.erase(std::begin(particles) + i);
				i--;
			}
		}
	}

	void Draw()
	{
		for (int i = 0; i < particles.size(); i++)
		{
			particles[i].Draw();
		}
	}
private:
	std::vector<Particle> particles;
};

class ParticleHandler
{
public:

	void AddEmiter(ParticleEmiter& pe) 
	{ 
		ParticleEmiter pe_ = pe;
		particles_emiters.push_back(pe_); 
	}

	void Update()
	{
		//if(particles_emiters.size())
			//printf("emitters amount:%d\n", particles_emiters.size());
		for (int i = 0; i < particles_emiters.size(); i++)
		{
			particles_emiters[i].Update();
			if (particles_emiters[i].GetSize() == 0)
				particles_emiters.erase(std::begin(particles_emiters) + i);
		}
	}
	
	void Draw()
	{
		for (int i = 0; i < particles_emiters.size(); i++)
		{
			particles_emiters[i].Draw();
		}
	}

private:
	std::vector<ParticleEmiter> particles_emiters;
};


ParticleEmiter particle_emiter__enemy_cancer_hit(5,0);
ParticleEmiter particle_emiter__enemy_koronavirus_hit(5,0);

class Player : public Mob
{
public:
	Player() : Mob(Mob::Player, 400, 225, 25, 1000)
	{
		velocity = { 0,0 };
		speed = 1;
		sprite_offset.x = -38 * pixel_scale / 2;
		sprite_offset.y = -38 * pixel_scale / 2;
	}

	void PlayHitAnimation() override
	{
		hit_timer = hit_timer_duration;
	}

	void Update(BulletHandler& bullet_handl)
	{
		const float delta_time = GetFrameTime();
		const float friction = 0.999;
		Vector2 d = { IsKeyDown(KEY_D) - IsKeyDown(KEY_A), IsKeyDown(KEY_S) - IsKeyDown(KEY_W) };

		velocity.x *= friction;
		velocity.y *= friction;

		velocity = { velocity.x + d.x * speed, velocity.y + d.y * speed };

		prev_position = position;
		position = { position.x + velocity.x * delta_time, position.y + velocity.y * delta_time } ;

		if (position.x > screenWidth + 50)
			position.x = -50;
		if (position.x < -50)
			position.x = screenWidth + 50;
		if (position.y > screenHeight + 50)
			position.y = -50;
		if (position.y < -50)
			position.y = screenHeight + 50;

#pragma region Enemy Bullets Update
		Vector2 bul_pos;
		Bullet* b = nullptr;
		for (int i = 0; i < bullet_handl.GetBulletAmount(); i++)
		{
			b = &bullet_handl.GetBullet(i);
			if (!b) continue;
			if (b->GetBulletType() != Bullet::Enemy) continue;
			bul_pos = b->GetPosition();
			d = { position.x - bul_pos.x, position.y - bul_pos.y };
			if (hypot(d.x, d.y) < radius + b->GetRadius())
			{
				// Hitted enemy bullet
				Vector2 norm_vec = NormalizeVec(d);
				uint8_t hit_flags = 0b0;
				hit_flags |= (uint8_t)Mob::HitFlags::HITTED_BY_BULLET;
				hit_flags |= (uint8_t)Mob::HitFlags::HITTED_BY_ENEMY;
				//printf("hit flags: [%d]\n", hit_flags);
				const float bullet_strength = 10;
				Hit(d.x * bullet_strength, d.y * bullet_strength, hit_flags);
				PlayHitAnimation();
				b->IncrimentHitted();
			}
		}

		Frame_Update(delta_time);
#pragma endregion

#ifdef ArrowShooting

		d = Vector2{ float(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)), float(IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP)) };
		if (d.x || d.y)
		if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_DOWN))
		{
			bullet_handl.PushBullet(Bullet(Bullet::Player, position, Vector2{ d.x * BULLET_SPEED, d.y * BULLET_SPEED }, BULLET_MAX_LIFE_TIME));
			//printf("bullet\n");
		}
#else
		d = Vector2{ position.x - GetMousePosition().x, position.y - GetMousePosition().y };
		if (d.x || d.y)
		{
			if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				d = NormalizeVec(d);
				bullet_handl.PushBullet(Bullet(Bullet::Player, position, Vector2{ -d.x * bullet_speed, -d.y * bullet_speed }, BULLET_MAX_LIFE_TIME));
			}
		}
#endif
	}

	void Draw()
	{
		const float delta_time = GetFrameTime();
#ifdef DrawSpeedomeeter
		float c = hypot(prev_position.x - position.x, prev_position.y - position.y);
		DrawText(TextFormat("speed: %.02f", c), 0, 0, 20, LIGHTGRAY);
		DrawText(TextFormat("(game)speed x: %.02f", velocity.x * delta_time), 0, 20, 20, LIGHTGRAY);
		DrawText(TextFormat("(game)speed y: %.02f", velocity.x * delta_time), 0, 40, 20, LIGHTGRAY);
#endif


#ifdef DrawSprites
		rect_dst.width = player_texture.width * pixel_scale * scale_x;
		rect_dst.height = player_texture.height * pixel_scale * scale_y;
		rect_dst.x = position.x;
		rect_dst.y = position.y;
		DrawTexturePro(player_texture, { 0,0,(float)player_texture.width, (float)player_texture.height }, rect_dst, { rect_dst.width / 2, rect_dst.height / 2 }, 0.0f, WHITE);
		//DrawTextureRec(player_texture, player_rectangle, Vector2{ position.x + sprite_offset.x, position.y + sprite_offset.y }, WHITE);
#endif
		DrawRing(position, GetRadius(), GetRadius() + 5, 0, 360, 0, Color{0,0,255,100});
	}
private:
	void Frame_Update(const float delta_time)
	{
		if (hit_timer > 0)
			hit_timer -= delta_time;
		if (hit_timer < 0)
			hit_timer = 0;

		scale_y = 1.0f - pow(hit_timer / hit_timer_duration, 2) * 0.9f;
		scale_x = 1.0f + pow(hit_timer / hit_timer_duration, 2) * 0.6f;
	}

	Vector2 prev_position;
	float speed;
	Vector2 sprite_offset;
	const float bullet_speed = 1750;


	float hit_timer;
	const float hit_timer_duration = 0.4f;
	Rectangle rect_dst;
	float scale_x = 1.0f, scale_y = 1.0f;
};

class Enemy : public Mob
{
public:

	Enemy(float x, float y) : Mob(Mob::Enemy, x, y, 25, 100)
	{
		push_strength = 100;
		velocity = { 0,0 };
		color = RED;
		color.a = 100;
		sprite_offset.x = -31 * pixel_scale / 2;
		sprite_offset.y = -26 * pixel_scale / 2;
	}

	void PlayHitAnimation() override
	{
		hit_timer = hit_timer_duration;
	}

	virtual void UpdateBullets(Bullet& bullet, ParticleHandler& particle_handl) = 0;

	virtual void Update(Mob& mob, BulletHandler& bullet_handl) = 0;

	virtual void Draw() = 0;

protected:

	void Collision_Update(Mob& mob, float& d, float& delta_x, float& delta_y, const float delta_time)
	{
		// Collision calculation
		delta_x = mob.GetPosition().x - position.x;
		delta_y = mob.GetPosition().y - position.y;
		d = hypot(delta_x, delta_y);

		if (d <= radius + mob.GetRadius())
		{
			const float norm_delta_x = (1.0f - delta_x) / radius * push_strength;
			const float norm_delta_y = (1.0f - delta_y) / radius * push_strength;

			uint8_t hit_flags = 0;
			if (mob.GetType() != GetType()) // if both mobs aren't enemies
				if (mob.GetType() == Mob::Type::Player) // if one of mobs is player
					hit_flags |= (uint8_t)Mob::HitFlags::HITTED_BY_ENEMY; // setting flag that player got hit by enemy
			mob.Hit(-norm_delta_x, -norm_delta_y, hit_flags);

			if(CanGetHurt())
			{
				mob.PlayHitAnimation();
				PlayHitAnimation();
			}

			hit_flags = 0;
			if (mob.GetType() != GetType())
				if (mob.GetType() == Mob::Type::Player)
					hit_flags |= (uint8_t)Mob::HitFlags::HITTED_BY_PLAYER; // setting flag that enemy got hit by player
			Hit(norm_delta_x * 5, norm_delta_y * 5, hit_flags);
		}

	}
	void Frame_Update(const float delta_time)
	{
		if (hit_timer > 0)
			hit_timer -= delta_time;
		if (hit_timer < 0)
			hit_timer = 0;

		scale_y = 1.0f - pow(hit_timer / hit_timer_duration, 2) * 0.9f;
		scale_x = 1.0f + pow(hit_timer / hit_timer_duration, 2) * 0.6f;
	}

	float push_strength;
	Color color;
	Vector2 sprite_offset;

	float hit_timer;
	const float hit_timer_duration = 0.4f;
	float scale_x, scale_y;
};

class Cancer : public Enemy
{
public:

	Cancer(float x, float y) : Enemy(x, y)
	{

	}

	void UpdateBullets(Bullet& bullet, ParticleHandler& particle_handl) override
	{
		if (bullet.GetBulletType() == Bullet::Enemy) return;

		const float delta_x = bullet.GetPosition().x - position.x;
		const float delta_y = bullet.GetPosition().y - position.y;
		const float d = hypot(delta_x, delta_y);

		if (d <= radius + bullet.GetRadius())
		{
			const float norm_delta_x = (1.0f - delta_x) / radius * push_strength;
			const float norm_delta_y = (1.0f - delta_y) / radius * push_strength;

			uint8_t hit_flags = 0;

			hit_flags |= (uint8_t)Mob::HitFlags::HITTED_BY_PLAYER;
			hit_flags |= (uint8_t)Mob::HitFlags::HITTED_BY_BULLET;
			hit_flags |= (uint8_t)Mob::HitFlags::MODIFIER_BULLET_HURT_MORE;

			Hit(norm_delta_x * 5, norm_delta_y * 5, hit_flags);

			PlayHitAnimation();

			bullet.IncrimentHitted();

			printf("position of emitter: X=%.02f Y=%.02f\n", position.x, position.y);
			particle_emiter__enemy_cancer_hit.SetPositionToEmitFrom(position);
			particle_emiter__enemy_cancer_hit.SetVelocityToEmitWith({ norm_delta_x, norm_delta_y });
			particle_handl.AddEmiter(particle_emiter__enemy_cancer_hit);

			//velocity.x += norm_delta_x / 2;
			//velocity.y += norm_delta_y / 2;
		}
	}

	void Update(Mob& mob, BulletHandler& bullet_handl) override
	{
		// Collision calculation
		const float delta_time = GetFrameTime();
		float d = 0, delta_x, delta_y;
		Collision_Update(mob, d, delta_x, delta_y, delta_time);

		Frame_Update(delta_time);

		velocity.y += JRAVITY_STRENGTH * 0.01;

		// Position & Velocity calculation
		velocity.x *= 0.999;
		velocity.y *= 0.999;

		if (velocity.x > MAX_VELOCITY)
			velocity.x = MAX_VELOCITY;
		if (velocity.y > MAX_VELOCITY)
			velocity.y = MAX_VELOCITY;

		position.x += velocity.x * delta_time;
		position.y += velocity.y * delta_time;


		if (position.x > screenWidth + 50)
			position.x = -50;
		if (position.x < -50)
			position.x = screenWidth + 50;
		if (position.y > screenHeight + 50)
			position.y = -50;
		if (position.y < -50)
			position.y = screenHeight + 50;
	}

	void Draw() override
	{
#ifdef DrawSprites
		enemy_rect_dst.width = enemy_cancer_texture.width * pixel_scale * scale_x;
		enemy_rect_dst.height = enemy_cancer_texture.height * pixel_scale * scale_y;
		enemy_rect_dst.x = position.x;
		enemy_rect_dst.y = position.y;
		DrawTexturePro(enemy_cancer_texture, { 0,0,(float)enemy_cancer_texture.width, (float)enemy_cancer_texture.height }, enemy_rect_dst, { enemy_rect_dst.width / 2, enemy_rect_dst.height / 2 }, 0.0f, WHITE);
#endif																																		  
		DrawRing(position, GetRadius(), GetRadius() + 5, 0, 360, 0, color);
	}
protected:
	Rectangle enemy_rect_dst;
};

class Koronavirus : public Enemy
{
public:

	Koronavirus(float x, float y) : Enemy(x, y)
	{

	}

	void UpdateBullets(Bullet& bullet, ParticleHandler& particle_handl) override
	{
		if (bullet.GetBulletType() == Bullet::Enemy) return;

		const float delta_x = bullet.GetPosition().x - position.x;
		const float delta_y = bullet.GetPosition().y - position.y;
		const float d = hypot(delta_x, delta_y);

		if (d <= radius + bullet.GetRadius())
		{
			const float norm_delta_x = (1.0f - delta_x) / radius * push_strength;
			const float norm_delta_y = (1.0f - delta_y) / radius * push_strength;

			uint8_t hit_flags = 0;

			hit_flags |= (uint8_t)Mob::HitFlags::HITTED_BY_PLAYER;
			hit_flags |= (uint8_t)Mob::HitFlags::HITTED_BY_BULLET;

			Hit(norm_delta_x * 5, norm_delta_y * 5, hit_flags);

			PlayHitAnimation();

			bullet.IncrimentHitted();

			printf("position of emitter: X=%.02f Y=%.02f\n", position.x, position.y);
			particle_emiter__enemy_koronavirus_hit.SetPositionToEmitFrom(position);
			particle_emiter__enemy_koronavirus_hit.SetVelocityToEmitWith({ norm_delta_x, norm_delta_y });
			particle_handl.AddEmiter(particle_emiter__enemy_koronavirus_hit);

			//velocity.x += norm_delta_x / 2;
			//velocity.y += norm_delta_y / 2;
		}
	}

	void Update(Mob& mob, BulletHandler& bullet_handl) override
	{
		const float delta_time = GetFrameTime();
		float d = 0, delta_x, delta_y;
		Collision_Update(mob, d, delta_x, delta_y, delta_time);

		if (d <= atack_radius + mob.GetRadius())
		{
			if (reload_timer >= shooting_period)
			{
				Vector2 norm_vec = NormalizeVec({ delta_x, delta_y });

				norm_vec.x *= BULLET_SPEED;
				norm_vec.y *= BULLET_SPEED;

				bullet_handl.PushBullet(Bullet(Bullet::Enemy, position, norm_vec, BULLET_MAX_LIFE_TIME * 2));
				reload_timer = 0;
			}
		}

		Frame_Update(delta_time);


		reload_timer += delta_time;

		velocity.y += JRAVITY_STRENGTH * 0.01;

		velocity.x += sin(reload_timer * 5) * 0.5;
		velocity.y += cos(reload_timer * 0.1) * 0.2;

		// Position & Velocity calculation
		velocity.x *= 0.999;
		velocity.y *= 0.999;

		if (velocity.x > MAX_VELOCITY)
			velocity.x = MAX_VELOCITY;
		if (velocity.y > MAX_VELOCITY)
			velocity.y = MAX_VELOCITY;

		position.x += velocity.x * delta_time;
		position.y += velocity.y * delta_time;


		if (position.x > screenWidth + 50)
			position.x = -50;
		if (position.x < -50)
			position.x = screenWidth + 50;
		if (position.y > screenHeight + 50)
			position.y = -50;
		if (position.y < -50)
			position.y = screenHeight + 50;
	}

	void Draw() override
	{
#ifdef DrawSprites
		//DrawTextureRec(enemy_koronavirus_texture, enemy_koronavirus_rectangle, Vector2{ position.x + sprite_offset.x, position.y + sprite_offset.y }, WHITE);
		enemy_rect_dst.width = enemy_koronavirus_texture.width * pixel_scale * scale_x;
		enemy_rect_dst.height = enemy_koronavirus_texture.height * pixel_scale * scale_y;
		enemy_rect_dst.x = position.x;
		enemy_rect_dst.y = position.y;
		DrawTexturePro(enemy_koronavirus_texture, { 0,0,(float)enemy_koronavirus_texture.width, (float)enemy_koronavirus_texture.height }, 
			enemy_rect_dst, { enemy_rect_dst.width / 2, enemy_rect_dst.height / 2 }, 0.0f, WHITE);

#endif
		DrawRing(position, GetRadius(), GetRadius() + 5, 0, 360, 0, color);
	}
private:
	float shooting_period = 4.0f;
	float reload_timer = 0.0f;
	float atack_radius = 500;
	Rectangle enemy_rect_dst;
};

int main()
{
	InitWindow(screenWidth, screenHeight, "Window");
	SetAudioStreamBufferSizeDefault(1024);
	InitAudioDevice();

	Player player;
	player_texture = LoadTexture("player.png");
	player_rectangle.width = player_texture.width;
	player_rectangle.height = player_texture.height;

	enemy_cancer_texture = LoadTexture("enemy.png");
	enemy_cancer_rectangle.width = enemy_cancer_texture.width;
	enemy_cancer_rectangle.height = enemy_cancer_texture.height;

	enemy_koronavirus_texture = LoadTexture("sprites\\enemy_koronavirus.png");
	enemy_koronavirus_rectangle.width = 33 * pixel_scale;
	enemy_koronavirus_rectangle.height = 35 * pixel_scale;

	BulletHandler bullet_handl;

	ParticleHandler particle_handl;

	Particle particle({0,0}, {0,0}, Particle::Texture, 1 + float(GetRandomValue(-1, 1)) * 0.5f);
	particle.SetScaleChangeFunc1(sqrt);
	particle.AddTexture("sprites\\bullet_enemy_cancer_0.png");
	particle.AddTexture("sprites\\bullet_enemy_cancer_1.png");
	particle.AddTexture("sprites\\bullet_enemy_cancer_2.png");
	for (int i = 0; i < particle_emiter__enemy_cancer_hit.GetSize(); i++)
	{
		particle_emiter__enemy_cancer_hit.SetParticle(i, particle);
	}
	Particle k_particle({ 0,0 }, { 0,0 }, Particle::Texture, 1);
	k_particle.SetScaleChangeFunc1(sqrt);
	k_particle.AddTexture("sprites\\hit_enemy_kvirus_0.png");
	k_particle.AddTexture("sprites\\hit_enemy_kvirus_1.png");
	k_particle.AddTexture("sprites\\hit_enemy_kvirus_2.png");
	k_particle.AddTexture("sprites\\hit_enemy_kvirus_3.png");
	for (int i = 0; i < particle_emiter__enemy_koronavirus_hit.GetSize(); i++)
	{
		particle_emiter__enemy_koronavirus_hit.SetParticle(i, k_particle);
	}

	std::vector<std::unique_ptr<Enemy>> enemies;
	for (int i = 0; i < AMOUNT_OF_ENEMIES; i++)
	{
		float x = GetRandomValue(0, screenWidth);
		float y = GetRandomValue(0, screenHeight);
		int rand_val = GetRandomValue(0, 1);
		if(rand_val == 0)
		{
			enemies.push_back(std::make_unique<Cancer>(x, y));
		}
		else //if(rand_val == 1)
		{
			enemies.push_back(std::make_unique<Koronavirus>(x, y));
		}
		//else
		//{
		//	Enemy enemy(x, y);
		//	enemies.push_back(std::make_unique<Enemy>(enemy));
		//}
	}

	//hit_sound = LoadSound("audio\\hurt.wav");
	hit_sound = LoadSound("hit.wav");
	SetSoundVolume(hit_sound, 0.2f);

	cursor_texture = LoadTexture("sprites\\cursor.png");
	cursor_rectangle.width = (float)cursor_texture.width * pixel_scale * pixel_scale;
	cursor_rectangle.height = (float)cursor_texture.height * pixel_scale * pixel_scale;


#ifdef DrawSelector
	Vector2 selector_pos = { 0,0 };
	bool selector_hovered_smth = false;
	float selector_life_timer = 0.0f;
	float selector_dist_from_center = 0.0f;
	Texture2D selector_corner_texture = LoadTexture("sprites\\selector_corner.png");
	Rectangle selector_src_rect = { 0,0,(float)selector_corner_texture.width, (float)selector_corner_texture.height };
	Rectangle selector_dst_rect = { 0,0,
		(float)selector_corner_texture.width * pixel_scale* pixel_scale, 
		(float)selector_corner_texture.height * pixel_scale* pixel_scale 
	};
#ifdef AdditionalSelectorOffsetFromCenterToMouse
	Vector2 selector_additional_offset = { 0,0 };
#endif
#endif

#ifdef BlockMouseInWindow
	Vector2 m_pos = {0,0};
#endif

#ifdef DynamicCursorRotation
	Vector2 m_prev_pos = { 0,0 };
	Vector2 mouse_position_delta = { 0,0 };
	float dynamic_mouse_angle = 0;
#endif
	int killed_enemies = 0;
	while (!WindowShouldClose())
	{

#ifdef BlockMouseInWindow
	#ifdef DynamicCursorRotation
			m_prev_pos = m_pos;
	#endif
		m_pos = GetMousePosition();
		
		if (m_pos.x < 0)	m_pos.x = 0;
		if (m_pos.y < 0)	m_pos.y = 0;
		if (m_pos.x > screenWidth)	m_pos.x = screenWidth;
		if (m_pos.y > screenHeight)	m_pos.y = screenHeight;

		SetMousePosition(m_pos.x,m_pos.y);
#endif

#ifdef DrawSelector
		selector_hovered_smth = false;
#endif

#ifdef ShowCursor
		HideCursor();
		cursor_rectangle.x = m_pos.x - cursor_texture.width / 2;
		cursor_rectangle.y = m_pos.y - cursor_texture.height / 2;

#ifdef DynamicCursorRotation
		mouse_position_delta.x += clamp(-100, 100, m_pos.x - m_prev_pos.x);
		mouse_position_delta.y += clamp(-100, 100, m_pos.y - m_prev_pos.y);
		const float max_angle_rotation = 30;
		//Vector2 delta_mouse = {};
		const float delta = mouse_position_delta.x / 100; //-1..1

		dynamic_mouse_angle = delta * max_angle_rotation;

		mouse_position_delta.x *= 0.99f;
		mouse_position_delta.y *= 0.99f;
#endif

#endif


		//PlaySound(s);
		player.Update(bullet_handl);
		player.Tick_Update();

		particle_handl.Update();

		for (int i = 0; i < enemies.size(); i++)
		{
			enemies[i]->Tick_Update();
		}

		bullet_handl.Update();

		if (enemies.size() < killed_enemies + 2)
		{
			float x = GetRandomValue(0, screenWidth);
			float y = screenHeight + 30;
			int rand_val = GetRandomValue(0, 1);
			if (rand_val == 0)
			{
				enemies.push_back(std::make_unique<Cancer>(x, y));
			}
			else //if(rand_val == 1)
			{
				enemies.push_back(std::make_unique<Koronavirus>(x, y));
			}
		}
		for (int i = 0; i < enemies.size(); i++)
		{
			for (int j = 0; j < bullet_handl.GetBulletAmount(); j++)
			{
				enemies[i]->UpdateBullets(bullet_handl.GetBullet(j), particle_handl);
			}
			enemies[i]->Update(player, bullet_handl);

#ifdef DrawSelector
			if (AreTouching(enemies[i]->GetPosition(), enemies[i]->GetRadius() * SelectorAccuracy, m_pos, 0))
			{
				selector_pos = enemies[i]->GetPosition();
				selector_hovered_smth = true;
				selector_life_timer += GetFrameTime();
				selector_dist_from_center = enemies[i]->GetRadius() * SelectorOffset;
			}
#endif

			if (!enemies[i]->IsAlive())
			{
				killed_enemies++;
				enemies.erase(std::begin(enemies) + i);
				i--;
			}
		}
		
		BeginDrawing();

			ClearBackground(RAYWHITE);
			DrawText(TextFormat("fps: %.02f", 1 / GetFrameTime()), 0, 80, 20, LIGHTGRAY);

			particle_handl.Draw();
			
			player.Draw();

			for (int i = 0; i < enemies.size(); i++)
				enemies[i]->Draw();

			player.DrawBars();

			for (int i = 0; i < enemies.size(); i++)
				enemies[i]->DrawBars();

			bullet_handl.Draw();

#ifdef DrawSelector
			//selector_dst_rect.x = -selector_dist_from_center;
			//selector_dst_rect.y = -selector_dist_from_center;

			if(selector_hovered_smth)
			{
				float angle = selector_life_timer / 2 * 360;
				selector_additional_offset.x = (selector_pos.x - m_pos.x) / SelectorAccuracy;
				selector_additional_offset.y = (selector_pos.y - m_pos.y) / SelectorAccuracy;
				selector_dst_rect.x = selector_pos.x - selector_additional_offset.x;
				selector_dst_rect.y = selector_pos.y - selector_additional_offset.y;
				DrawTexturePro(selector_corner_texture, selector_src_rect, selector_dst_rect, { selector_dist_from_center, selector_dist_from_center }, angle, WHITE);
				DrawTexturePro(selector_corner_texture, selector_src_rect, selector_dst_rect, { selector_dist_from_center, selector_dist_from_center }, angle+90, WHITE);
				DrawTexturePro(selector_corner_texture, selector_src_rect, selector_dst_rect, { selector_dist_from_center, selector_dist_from_center }, angle+180, WHITE);
				DrawTexturePro(selector_corner_texture, selector_src_rect, selector_dst_rect, { selector_dist_from_center, selector_dist_from_center }, angle+270, WHITE);
			}
#endif

#ifdef DynamicCursorRotation
			DrawTexturePro(cursor_texture, { 0,0,(float)cursor_texture.width,(float)cursor_texture.height }, cursor_rectangle, 
				{ (float)cursor_texture.width * pixel_scale / 2.0f,(float)cursor_texture.height * pixel_scale / 2.0f }, dynamic_mouse_angle, WHITE);
#else

	#ifdef DrawSprites
				DrawTexturePro(cursor_texture, { 0,0,(float)cursor_texture.width,(float)cursor_texture.height }, cursor_rectangle, { 0,0 }, 0.0f, WHITE);
	#endif
#endif

		EndDrawing();
	}
	
	UnloadSound(hit_sound);
	CloseAudioDevice();
	CloseWindow();
}