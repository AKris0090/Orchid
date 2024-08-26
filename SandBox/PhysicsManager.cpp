#include "PhysicsManager.h"

void PhysicsManager::setup() {
	pFoundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, defaultAllocatorCallback, defaultErrorCallback);
	if (!pFoundation_) {
		std::cout << "physics foundation failed" << std::endl;
		std::_Xruntime_error("physics foundation failed");
	}

	pPVirtDebug = PxCreatePvd(*pFoundation_);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10000);
	if (transport == NULL) {
		std::cout << "failed IP server for physx debug" << std::endl;
		std::_Xruntime_error("failed IP server for physx debug");
	}
	pPVirtDebug->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	pPhysics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *pFoundation_, physx::PxTolerancesScale(), recordMemoryAllocations, pPVirtDebug);
	if (!pPhysics_) {
		std::cout << "physics instance failed" << std::endl;
		std::_Xruntime_error("physics instance failed");
	}

	physx::PxSceneDesc sceneDescription(pPhysics_->getTolerancesScale());
	sceneDescription.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	pDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
	sceneDescription.cpuDispatcher = pDispatcher;
	sceneDescription.filterShader = physx::PxDefaultSimulationFilterShader;
	pScene = pPhysics_->createScene(sceneDescription);

	physx::PxPvdSceneClient* pvdSceneClient = pScene->getScenePvdClient();
	if (pvdSceneClient) {
		pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	pMaterial = pPhysics_->createMaterial(0.5f, 0.5f, 0.15f);
}

void PhysicsManager::addPlane() {
	physx::PxShapeFlags shapeFlags(physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE);

	physx::PxRigidStatic* groundplane = PxCreatePlane(*pPhysics_, physx::PxPlane(0, 1, 0, 99), *pMaterial);

	pScene->addActor(*groundplane);
}

void PhysicsManager::addCubeToGameObject(GameObject* gameObject, physx::PxVec3 globalTransform, float halfExtent) {
	physx::PxShapeFlags shapeFlags(physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE);

	physx::PxShape* cubeShape = pPhysics_->createShape(physx::PxBoxGeometry(physx::PxReal(halfExtent), physx::PxReal(halfExtent), physx::PxReal(halfExtent)), &pMaterial, 1, true, shapeFlags);
	physx::PxRigidDynamic* body2 = pPhysics_->createRigidDynamic(physx::PxTransform(globalTransform));
	body2->attachShape(*cubeShape);
	gameObject->physicsActor = body2;
	gameObject->pShape_ = cubeShape;
	physx::PxRigidBodyExt::updateMassAndInertia(*body2, 10.0f);
	pScene->addActor(*body2);

	cubeShape->release();
}

void PhysicsManager::addShapeToGameObject(GameObject* gameObject, physx::PxVec3 globalTransform, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3& scale) {
	physx::PxShapeFlags shapeFlags(physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE);

	//physx::PxShape* shape = createPhysicsFromMesh(gameObject, vertices, indices, pMaterial, scale);
	std::vector<physx::PxShape*> shapes = createPhysicsFromMesh(gameObject, vertices, indices, pMaterial, scale);
	physx::PxRigidStatic* body = pPhysics_->createRigidStatic(physx::PxTransform(globalTransform));
	gameObject->physicsActor = body;
	//gameObject->pShape_ = shape;
	for (auto& shape : shapes) {
		body->attachShape(*shape);
		shape->release();
	}
	pScene->addActor(*body);
}

void PhysicsManager::recursiveAddToList(GameObject* g, std::vector<physx::PxVec3>& pxVertices, std::vector<uint32_t>& pxIndices, GLTFObj::SceneNode* node, std::vector<Vertex>& vertices) {
	for (auto& mesh : node->meshPrimitives) {
		for (int i = g->renderTarget->globalFirstVertex; i < g->renderTarget->globalFirstVertex + g->renderTarget->totalVertices_; i++) {
			pxVertices.push_back(physx::PxVec3(vertices.at(i).pos.x, vertices.at(i).pos.y, vertices.at(i).pos.z));
			pxIndices.push_back(i - g->renderTarget->globalFirstVertex);
		}
	}

	for (auto& childNode : node->children) {
		recursiveAddToList(g, pxVertices, pxIndices, childNode, vertices);
	}
}

std::vector<physx::PxShape*> PhysicsManager::createPhysicsFromMesh(GameObject* g, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, physx::PxMaterial* material, glm::vec3& scale) {
	//for(int i = g->renderTarget->getFirstVertex(); i < g->renderTarget->getFirstVertex() + g->renderTarget->getTotalVertices(); i++) {
	//	pxVertices.push_back(physx::PxVec3(vertices.at(i).pos.x, vertices.at(i).pos.y, vertices.at(i).pos.z));
	//}

	//for (int i = g->renderTarget->getFirstIndex(); i < g->renderTarget->getFirstIndex() + g->renderTarget->getTotalIndices(); i++) {
	//	pxIndices.push_back(indices.at(i));
	//}

	//for (int i = g->renderTarget->getFirstIndex(); i < g->renderTarget->getFirstIndex() + g->renderTarget->getTotalIndices(); i++) {
	//	pxIndices.push_back(pxIndices.size());
	//	Vertex v = vertices.at(indices.at(i) + g->renderTarget->getFirstVertex());
	//	pxVertices.push_back(physx::PxVec3(v.pos.x, v.pos.y, v.pos.z));
	//}

	std::vector<physx::PxShape*> shapes;

	for (auto& drawCall : g->renderTarget->opaqueDraws) {
		for (auto& dC : drawCall.second) {
			std::vector<physx::PxVec3> pxVertices;
			std::vector<uint32_t> pxIndices;
			int count = 0;
			for (int i = dC->indirectInfo.firstIndex; i < dC->indirectInfo.firstIndex + dC->indirectInfo.numIndices; i++) {
				glm::mat4 trueModel = g->renderTarget->localModelTransform * dC->worldTransformMatrix;
				Vertex vert = vertices.at(indices.at(i) + g->renderTarget->globalFirstVertex);
				glm::vec4 p = glm::vec4(vert.pos.x, vert.pos.y, vert.pos.z, 1.0f) * trueModel;
				pxVertices.push_back(physx::PxVec3(p.x, p.y, p.z));
				pxIndices.push_back(count);
				count++;
			}

			physx::PxTriangleMeshDesc meshDescription;
			meshDescription.points.count = pxVertices.size();
			meshDescription.points.data = pxVertices.data();
			meshDescription.points.stride = sizeof(physx::PxVec3);

			meshDescription.triangles.count = pxVertices.size() / 3;
			meshDescription.triangles.data = pxIndices.data();
			meshDescription.triangles.stride = 3 * sizeof(physx::PxU32);

			assert(meshDescription.isValid());

			physx::PxTolerancesScale toleranceScale;
			physx::PxCookingParams params(toleranceScale);

			params.midphaseDesc = physx::PxMeshMidPhase::eBVH33;

			bool skipMeshCleanup = false;
			bool skipEdgeData = false;
			bool cookingPerformance = false;
			bool meshSizePerfTradeoff = true;

			params.suppressTriangleMeshRemapTable = true;

			params.meshPreprocessParams |= static_cast<physx::PxMeshPreprocessingFlags>(physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);

			params.meshPreprocessParams &= ~static_cast<physx::PxMeshPreprocessingFlags>(physx::PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);

			params.midphaseDesc.mBVH33Desc.meshCookingHint = physx::PxMeshCookingHint::eSIM_PERFORMANCE;

			params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = 0.0f;

			physx::PxTriangleMesh* triMesh = NULL;

			triMesh = PxCreateTriangleMesh(params, meshDescription, pPhysics_->getPhysicsInsertionCallback());


			physx::PxMeshGeometryFlags flags(~physx::PxMeshGeometryFlag::eDOUBLE_SIDED);
			physx::PxTriangleMeshGeometry geo(triMesh, physx::PxMeshScale(physx::PxVec3(scale.x, scale.y, scale.z)), flags);

			physx::PxShapeFlags shapeFlags(physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE);
			physx::PxShape* shape = pPhysics_->createShape(geo, *(material), shapeFlags);
			shapes.push_back(shape);
		}
	}

	return shapes;
}


void PhysicsManager::loopUpdate(AnimatedGameObject* playerAnimObject, std::vector<GameObject*> gameObjects, std::vector<AnimatedGameObject*> animatedGameObjects, PlayerObject* player, FPSCam* cam, float deltaTime) {
	if (deltaTime > 0) {
		pScene->simulate(deltaTime);

		pScene->fetchResults(true);
	}

	for (GameObject* g : gameObjects) {
		if (g->isDynamic && g->physicsActor != nullptr) {
			glm::mat4 transform = g->toGLMMat4(g->physicsActor->getGlobalPose());
			g->setTransform(transform * g->transform.to_matrix());
		}
	}
	for (AnimatedGameObject* g : animatedGameObjects) {
		if (g->isDynamic && g->physicsActor != nullptr && !g->isPlayerObj) {
			glm::mat4 transform = g->toGLMMat4(g->physicsActor->getGlobalPose());
			g->setTransform(transform * g->transform.to_matrix());
		}
	}
}

void PhysicsManager::shutDown() {
	pDispatcher->release();
	pPVirtDebug->release();
	pMaterial->release();
	pScene->release();
	pPhysics_->release();
	pFoundation_->release();
}