#include "transform.h"
#include "immediate_gui.h"

Transform::~Transform()
{
	mTransform->destroy();
}

optix::Matrix4x4 Transform::get_matrix()
{
	mHasChanged = false;
	auto mtx = optix::Matrix4x4::identity();
	mtx = mtx.scale(mScale);
	mtx = mtx.rotate(mRotationAngle, mRotationAxis);
	mtx = mtx.translate(mTranslation);
	return mtx;
}

bool Transform::on_draw()
{
	mHasChanged |= ImmediateGUIDraw::InputFloat3((std::string("Translate##TranslateTransform") + std::to_string(id)).c_str(), &mTranslation.x , 2);

	static optix::float3 val = mRotationAxis;
	if (ImmediateGUIDraw::InputFloat3((std::string("Rotation axis##RotationAxisTransform") + std::to_string(id)).c_str(), &val.x, 2))
	{
		mHasChanged = true;
		mRotationAxis = optix::normalize(val);
	}

	mHasChanged |= ImmediateGUIDraw::InputFloat((std::string("Rotation angle##RotationAngleTransform") + std::to_string(id)).c_str(), &mRotationAngle, 2);
	mHasChanged |= ImmediateGUIDraw::InputFloat3((std::string("Scale##ScaleTransform") + std::to_string(id)).c_str(), &mScale.x, 2);
	return mHasChanged;
}

bool Transform::has_changed() const
{
	return mHasChanged;
}

void Transform::translate(const optix::float3 & t)
{
	mHasChanged = true;
	mTranslation = t;
}

void Transform::rotate(float angle, const optix::float3 & axis)
{
	mHasChanged = true;
	mRotationAngle = angle;
	mRotationAxis = axis;
}

void Transform::scale(const optix::float3 & s)
{
	mHasChanged = true;
	mScale = s;
}

Transform::Transform(optix::Context & ctx)
{
	context = ctx;
	if(mTransform.get() == nullptr)
	{
		mTransform = context->createTransform();
	}
}

void Transform::load()
{
	mTransform->setMatrix(false, get_matrix().getData(), nullptr);
}
