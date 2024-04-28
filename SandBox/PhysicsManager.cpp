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

	pTolerancesScale_.length = 100;
	pTolerancesScale_.speed = 981;	

	pPhysics_ = PxCreatePhysics(PX_PHYSICS_VERSION, *pFoundation_, pTolerancesScale_, recordMemoryAllocations, pPVirtDebug);
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

	pMaterial = pPhysics_->createMaterial(0.5f, 0.5f, 0.6f);
	physx::PxRigidStatic* groundPlane = PxCreatePlane(*pPhysics_, physx::PxPlane(0, 1, 0, 0), *pMaterial);
	pScene->addActor(*groundPlane);
}

void PhysicsManager::addShapes(MeshHelper* mesh, std::vector<GameObject*> gameObjects) {
	//float halfExtent = 0.5f;
	//physx::PxShape* shape = pPhysics_->createShape(physx::PxBoxGeometry(halfExtent, halfExtent, halfExtent), *pMaterial);
	//uint32_t size = 15;
	//physx::PxTransform t(physx::PxVec3(0));
	//for (physx::PxU32 i = 0; i < size; i++)
	//{
	//	for (physx::PxU32 j = 0; j < size - i; j++)
	//	{
	//		physx::PxTransform localTm(physx::PxVec3(physx::PxReal(j * 2) - physx::PxReal(size - i), physx::PxReal(i * 2 + 1), 0) * halfExtent);
	//		physx::PxRigidDynamic* body = pPhysics_->createRigidDynamic(t.transform(localTm));
	//		body->attachShape(*shape);
	//		physx::PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	//		pScene->addActor(*body);
	//	}
	//}

	// creating the simulation - move to separate functions
	physx::PxTransform t(physx::PxVec3(0.0f, 0.0f, 0.0f));

	//physx::PxShape* shape2 = pPhysics_->createShape(physx::PxBoxGeometry(15, 15, 5), *pMaterial);
	physx::PxTransform localTm2(physx::PxVec3(0, 400, 0));
	//physx::PxRigidStatic* nonMoving = pPhysics_->createRigidStatic(t.transform(localTm2));
	//nonMoving->attachShape(*shape2);
	//pScene->addActor(*nonMoving);

	physx::PxShapeFlags shapeFlags(physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE);

	physx::PxShape* shape = createPhysicsFromMesh(mesh, pMaterial, glm::vec3(0.008f));
	physx::PxRigidStatic* body = pPhysics_->createRigidStatic(t);
	body->attachShape(*shape);
	pScene->addActor(*body);

	gameObjects[1]->physicsActor = body;
	gameObjects[1]->pShape_ = shape;

	physx::PxShape* sphereShape = pPhysics_->createShape(physx::PxBoxGeometry(physx::PxReal(0.8f), physx::PxReal(0.8f), physx::PxReal(0.8f)), &pMaterial, 1, true, shapeFlags);
	physx::PxRigidDynamic* body2 = pPhysics_->createRigidDynamic(localTm2);
	body2->attachShape(*sphereShape);
	physx::PxRigidBodyExt::updateMassAndInertia(*body2, 10.0f);
	pScene->addActor(*body2);


	gameObjects[0]->physicsActor = body2;
	gameObjects[0]->pShape_ = sphereShape;

	//shape->release();
	//shape2->release();
}

physx::PxShape* PhysicsManager::createPhysicsFromMesh(MeshHelper* mesh, physx::PxMaterial* material, glm::vec3 scale) {
	std::vector<physx::PxVec3> pxVertices;
	std::vector<uint32_t> pxIndices;

	for (MeshHelper::Vertex& vertex : mesh->vertices_) {
		pxVertices.push_back(physx::PxVec3(vertex.pos.x, vertex.pos.y, vertex.pos.z));
		pxIndices.push_back(pxIndices.size());
	}

	physx::PxTriangleMeshDesc meshDescription;
	meshDescription.points.count = pxVertices.size();
	meshDescription.points.data = pxVertices.data();
	meshDescription.points.stride = sizeof(physx::PxVec3);

	meshDescription.triangles.count = pxVertices.size() / 3;
	meshDescription.triangles.data = mesh->indices_.data();
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
	shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);

	return shape;
}


void PhysicsManager::loopUpdate(std::vector<GameObject*> gameObjects) {
	pScene->simulate(1.0f / 60.0f);

	pScene->fetchResults(true);

	for (GameObject* g : gameObjects) {
		glm::mat4 transform = g->toGLMMat4(g->physicsActor->getGlobalPose());
		//g->transform.position = g->PxVec3toGlmVec3(g->physicsActor->getGlobalPose().p);
		//std::cout << "Cube Position" << g->physicsActor->getGlobalPose().p.x << " , " << g->physicsActor->getGlobalPose().p.y << " , " << g->physicsActor->getGlobalPose().p.z << std::endl;
		g->renderTarget->modelTransform =  transform * g->transform.to_matrix();
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