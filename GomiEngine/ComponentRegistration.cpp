#include "ComponentRegistry.h"
#include "ComponentType.h"

// 新しいコンポーネントを作ったら、ここに名前と項目を足すとシーンファイルで使えるようになる
void registerBuiltinComponents()
{
	static bool done = false;
	if (done) return;
	done = true;

	auto& r = ComponentRegistry::getInstance();

	r.add<TransformComponent>("Transform")
		.field("position", &TransformComponent::position)
		.field("scale", &TransformComponent::scale)
		.field("rotation", &TransformComponent::rotation);

	r.add<RendererComponent>("Renderer")
		.field("path", &RendererComponent::path)
		.field("layer", &RendererComponent::layer)
		.field("order", &RendererComponent::order)
		.field("enable", &RendererComponent::enable)
		.field("useCamera", &RendererComponent::useCamera);

	r.add<PhysicsComponent>("Physics")
		.field("gravVel", &PhysicsComponent::gravVel)
		.field("extraVel", &PhysicsComponent::extraVel)
		.field("gravity", &PhysicsComponent::gravity)
		.field("maxGravity", &PhysicsComponent::maxGravity)
		.field("decay", &PhysicsComponent::decay)
		.field("isGround", &PhysicsComponent::isGround);

	r.add<PlayerControlComponent>("PlayerControl")
		.field("actions", &PlayerControlComponent::actions);

	r.add<MoveComponent>("Move")
		.field("inputVel", &MoveComponent::inputVel)
		.field("physVel", &MoveComponent::physVel)
		.field("result", &MoveComponent::result)
		.field("speed", &MoveComponent::speed)
		.field("maxSpeed", &MoveComponent::maxSpeed)
		.field("stopDamp", &MoveComponent::stopDamp)
		.field("turnBrake", &MoveComponent::turnBrake);

	r.add<ForwardMoveComponent>("ForwardMove")
		.field("speed", &ForwardMoveComponent::speed)
		.field("enable", &ForwardMoveComponent::enable)
		.field("facing", &ForwardMoveComponent::facing)
		.field("turnSpeed", &ForwardMoveComponent::turnSpeed)
		.field("additive", &ForwardMoveComponent::additive)
		.field("pulse", &ForwardMoveComponent::pulse);

	r.add<CameraFollowComponent>("CameraFollow")
		.field("offset", &CameraFollowComponent::offset)
		.field("x", &CameraFollowComponent::x)
		.field("y", &CameraFollowComponent::y);

	r.add<CollisionComponent>("Collision")
		.field("type", &CollisionComponent::type)
		.field("size", &CollisionComponent::size)
		.field("radius", &CollisionComponent::radius)
		.field("offset", &CollisionComponent::offset);

	r.add<EntityTagComponent>("EntityTag")
		.field("tags", &EntityTagComponent::tags);

	r.add<BackGroundComponent>("BackGround")
		.field("asset", &BackGroundComponent::asset)
		.field("origin", &BackGroundComponent::origin)
		.field("parallax", &BackGroundComponent::parallax)
		.field("layer", &BackGroundComponent::layer)
		.field("order", &BackGroundComponent::order)
		.field("enable", &BackGroundComponent::enable)
		.field("margin", &BackGroundComponent::margin);
}
