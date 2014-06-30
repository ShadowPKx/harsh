  /*=================================================================}
 /  <<<<<<<<<<<<<<<<<<<<<--- Harsh Engine --->>>>>>>>>>>>>>>>>>>>>>  }
/  e-mail  : xproger@list.ru                                         }
{  github  : https://github.com/xproger/harsh                        }
{====================================================================}
{ LICENSE:                                                           }
{ Copyright (c) 2013, Timur "XProger" Gagiev                         }
{ All rights reserved.                                               }
{                                                                    }
{ Redistribution and use in source and binary forms, with or without /
{ modification, are permitted under the terms of the BSD License.   /
{=================================================================*/
#include "utils.h"
#include "scene.h"
#include "core.h"

//{ Camera
Camera::Camera(Stream *stream) : SceneNode(NULL, stream) {
    if (!stream) return;
    mode  = stream->getInt();
    zNear = stream->getFloat();
    zFar  = stream->getFloat();
    FOV   = stream->getFloat();
    dist  = stream->getFloat();
    stream->getCopy(&pos, sizeof(vec3));
    stream->getCopy(&angle, sizeof(vec3));
    stream->getCopy(&target, sizeof(vec3));
}

Camera::~Camera() {
    //
}

void Camera::update() {
    updateMatrix();
    updatePlanes();
}

void Camera::updateMatrix() {
	mView.identity();

    switch (mode) {
        case CAMERA_MODE_GAME :
        case CAMERA_MODE_FREE :
            mView.rotate(-angle.z, vec3(0, 0, 1));
            mView.rotate(-angle.x, vec3(1, 0, 0));
            mView.rotate(-angle.y, vec3(0, 1, 0));
            mView.translate(vec3(-pos.x, -pos.y, -pos.z));
            break;
        case CAMERA_MODE_TARGET :
            mView.translate(vec3(0, 0, -dist));
            mView.rotate(-angle.z, vec3(0, 0, 1));
            mView.rotate(-angle.x, vec3(1, 0, 0));
            mView.rotate(-angle.y, vec3(0, 1, 0));
            mView.translate(vec3(-pos.x, -pos.y, -pos.z));
            break;
        case CAMERA_MODE_LOOKAT :
            mView.lookAt(pos, target, vec3(0, 1, 0));
            break;
    }
	matrix = mView.inverse();

    mProj.identity();
    mProj.perspective(FOV, (float)Render::width / (float)Render::height, zNear, zFar, Render::width > Render::height);
	mViewProj = mProj * mView;
}

void setPlane(vec4 &p, float x, float y, float z, float w) {
    float len = 1.0f / sqrtf(x*x + y*y + z*z);
    p.x = x * len;
    p.y = y * len;
    p.z = z * len;
    p.w = w * len;
}

void Camera::updatePlanes() {
	mat4 &m = mViewProj;
    setPlane(planes[0], m.e30 - m.e00, m.e31 - m.e01, m.e32 - m.e02, m.e33 - m.e03); // right
    setPlane(planes[1], m.e30 + m.e00, m.e31 + m.e01, m.e32 + m.e02, m.e33 + m.e03); // left
    setPlane(planes[2], m.e30 - m.e10, m.e31 - m.e11, m.e32 - m.e12, m.e33 - m.e13); // top
    setPlane(planes[3], m.e30 + m.e10, m.e31 + m.e11, m.e32 + m.e12, m.e33 + m.e13); // bottom
    setPlane(planes[4], m.e30 - m.e20, m.e31 - m.e21, m.e32 - m.e22, m.e33 - m.e23); // near
    setPlane(planes[5], m.e30 + m.e20, m.e31 + m.e21, m.e32 + m.e22, m.e33 + m.e23); // far
}

void Camera::debugUpdate(float speed) {
	if (mode == CAMERA_MODE_FREE || mode == CAMERA_MODE_TARGET)
		if (Core::input->touch[0].down) {
			vec2 delta = Core::input->touch[0].pos - Core::input->touch[0].start;
			angle.x -= delta.y * 0.01f;
			angle.y -= delta.x * 0.01f;
			angle.x = _min(_max(angle.x, -_PI *0.5f + _EPS), _PI * 0.5f - _EPS);
			Core::input->touch[0].start = Core::input->touch[0].pos;
		}

	if (mode == CAMERA_MODE_FREE) {
		vec3 dir = vec3(sinf(angle.y - _PI) * cosf(-angle.x), -sinf(-angle.x), cosf(angle.y - _PI) * cosf(-angle.x));
		vec3 v = vec3(0, 0, 0);

		if (Core::input->down[IK_W]) v += dir;
		if (Core::input->down[IK_S]) v -= dir;
		if (Core::input->down[IK_D]) v += dir.cross(vec3(0, 1, 0));
		if (Core::input->down[IK_A]) v -= dir.cross(vec3(0, 1, 0));

		if (Core::input->touch[1].down) {
//			vec2 d = Core::input->touch[1].pos - Core::input->touch[1].start;
			v += dir;
			/*
			if (Core::input->key['W']) v += dir;
			if (Core::input->key['S']) v -= dir;
			if (Core::input->key['D']) v += dir.cross(vec3(0, 1, 0));
			if (Core::input->key['A']) v -= dir.cross(vec3(0, 1, 0));
			*/
		}

		pos += v.normal() * (Core::deltaTime * speed);
	}

	if (mode == CAMERA_MODE_TARGET && Core::input->touch[1].down) {
		dist -= (Core::input->touch[1].pos.y - Core::input->touch[1].start.y) * 0.01f;
		Core::input->touch[1].start = Core::input->touch[1].pos;
	}
}

void Camera::setup() {
	Render::params.mViewProj = mViewProj;
	Render::params.camera.pos = matrix.inverse().getPos();
}

bool Camera::checkVisible(const Box &v) {
	return true;
    for (int i = 0; i < 6; i++)
        if (planes[i].dot(v.max) < 0 &&
            planes[i].dot(vec3(v.min.x, v.max.y, v.max.z)) < 0 &&
            planes[i].dot(vec3(v.max.x, v.min.y, v.max.z)) < 0 &&
            planes[i].dot(vec3(v.min.x, v.min.y, v.max.z)) < 0 &&
            planes[i].dot(vec3(v.max.x, v.max.y, v.min.z)) < 0 &&
            planes[i].dot(vec3(v.min.x, v.max.y, v.min.z)) < 0 &&
            planes[i].dot(vec3(v.max.x, v.min.y, v.min.z)) < 0 &&
            planes[i].dot(v.min) < 0)
            return false;
    return true;
}

vec3 Camera::toWorld(const vec2 &screenPos, const vec4 &plane) {
	vec2 p = vec2(screenPos.x, Render::height - screenPos.y) / vec2(Render::width, Render::height) * 2 - vec2(1, 1);
//	vec4 v = mViewProj.inverse() * vec4(p.x, p.y, -1, 1);

	vec4 v;
	v = mProj.inverse() * vec4(p.x, p.y, -1, 1);
	v = mView.inverse() * vec4(v.x, v.y, -1, 0);

	vec3 dir = vec3(v.x, v.y, v.z).normal();
	
	vec3 n = vec3(plane.x, plane.y, plane.z);
	float t = dir.dot(n);
	if (t == 0) return vec3(0, 0, 0);
	t = -(pos.dot(n) - plane.w) / t;
	return pos + dir * t;
}

vec4 Camera::pProject(const vec3 &pos) {
	return mViewProj * vec4(pos, 1.0f);
}

vec3 Camera::pUnProject(const vec4 &pos) {
	return (mViewProj.inverse() * pos).xyz();
}


//}

//{ Scene Node
SceneNode::SceneNode(SceneNode *parent, Stream *stream) : parent(parent), child(0), prev(0), next(0), rel_bbox(), bbox(), rel_matrix(1), matrix(1) {
    if (!stream) return;
/*
    rel_matrix = *(mat4*)stream->getData(sizeof(rel_matrix));
    rel_bbox = *(Box*)stream->getData(sizeof(rel_bbox));

	int count = stream->getInt();
	while (count > 0) {
		switch (stream->getInt()) {   // Node Type
			case NT_GROUP :
				add(new SceneNode(this, stream));
				break;
			case NT_MODEL :
				add(new Model(this, stream->getInt()));
				break;
			case NT_MESH :
				add(new Mesh(this, stream->getInt()));
				break;
			case NT_WALKMAP :
				//add(new Mesh(this, stream));
				break;
		}
	}
*/
}

SceneNode::~SceneNode() {
    remove();
    delete child;
}

void SceneNode::setParent(SceneNode *parent) {
    remove();
    if (parent)
        parent->add(this);
}

void SceneNode::setMatrix(const mat4 &rel_matrix) {
    this->rel_matrix = rel_matrix;
    matrix = parent ? parent->matrix * rel_matrix : rel_matrix;
    updateBounds();
// update childs matrices & bounds
    SceneNode *c = child;
    while (c) {
        c->setMatrix(c->rel_matrix);
        c = c->next;
    }
}

void SceneNode::updateBounds() {
    vec3 &min = rel_bbox.min,
         &max = rel_bbox.max;

    vec3 v[8] = {
        matrix * vec3(min.x, max.y, max.z),
        matrix * vec3(max.x, min.y, max.z),
        matrix * vec3(min.x, min.y, max.z),
        matrix * vec3(max.x, max.y, min.z),
        matrix * vec3(min.x, max.y, min.z),
        matrix * vec3(max.x, min.y, min.z),
        matrix * min,
        matrix * max,
    };

    bbox.min = bbox.max = v[0];
    for (int i = 1; i < 8; i++) {
        bbox.min = bbox.min.vmin(v[i]);
        bbox.max = bbox.max.vmax(v[i]);
    }
}

void SceneNode::update() {
    SceneNode *c = child;
    while (c) {
        c->update();
        c = c->next;
    }
}

void SceneNode::render() {
    SceneNode *c = child;
    while (c) {
		if (c->visible)
			c->render();
        c = c->next;
    }
}

void SceneNode::add(SceneNode *n) {
    if (child)
        child->prev = n;
    n->next = child;
    n->prev = NULL;
    child = n;
	n->updateBounds();
}

void SceneNode::remove() {
    if (next)
        next->prev = prev;

    if (prev) {
        prev->next = next;
    } else
        if (parent && parent->child == this)
            parent->child = next;

    parent = next = prev = NULL;
    rel_matrix = mat4(1);
}
//}

//{ Mesh Node
Mesh::Mesh(SceneNode *parent, Hash hash) : SceneNode(parent, NULL) {
	res = MeshRes::load(NULL, hash);
	res->valid();
	rel_bbox = res->bbox;
}

Mesh::~Mesh() {
	if (res) res->free();
}

void Mesh::render() {
	if (!res || !res->valid()) return;
	Shader::setMatrixModel(matrix);
/*
	if (material == Core::scene->mEdge) {
		glLineWidth(3.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
	}
*/
	if (material->bind())
		Render::drawTriangles(res->iBuffer, res->vBuffer, 0, res->iCount / 3);
/*
	if (material == Core::scene->mEdge) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
*/
}
//}

//{ Model Node
/*
Model::Model(SceneNode *parent, Hash hash) {
	Stream *stream = new Stream(hash);
	SceneNode(parent, stream->size > 0 ? stream : NULL);
	delete stream;
}
*/
//}

//{ Scene
Scene::Scene() : SceneNode(NULL, NULL) {
	//
}

Scene::~Scene() {
	//
}

void Scene::load(const char *name) {
	Stream *stream = new Stream(Stream::getHash(name));
	Mesh *mesh;

	int count = stream->getInt();
	while (count > 0) {
		int nodeType = stream->getInt();
		switch (nodeType) {
			case 'm' :
				mesh = new Mesh(this, stream->getInt());
				stream->getCopy(&mesh->rel_matrix, sizeof(mesh->rel_matrix));
				mesh->matrix = mesh->rel_matrix;
				mesh->material = new Material(Stream::getHash("material/none.xmt"));
				add(mesh);
				break;
			default :
				stream->seek(sizeof(Hash) + sizeof(mat4));
		}
		count--;
	}

	delete stream;
}

void Scene::checkVisible() {
    SceneNode *c = child;
    while (c) {
		c->visible = Core::scene->camera->checkVisible(c->bbox);
        c = c->next;
    }
}

void Scene::update() {
	camera->update();
	SceneNode::update();
}

void Scene::render() {
	camera->setup();

	Shader::setMatrixViewProj(Render::params.mViewProj);
	Shader::setViewPos(Render::params.camera.pos);

	checkVisible();

	Render::resetStates();

	// list = collect(RENDER_OPAQUE)
	// list->render()

	// list = collect(RENDER_OPACITY)
	// list->sort(camera->pos)
	// list->render()


//	Render::mode = RENDER_OPAQUE;
	Render::statSetTex = 0;
	Render::statTriCount = 0;
    SceneNode::render();
//	LOG("%d\n", Render::statSetTex);
//	LOG("%d\n", Render::statTriCount);

//	Render::mode = RENDER_OPACITY;
//    SceneNode::render();
//	Render::mode = RENDER_OPAQUE;

//    Mesh::sortRender();
}
//}

