namespace JD {


	struct ShadowPushConstants {
		uint32_t instanceBaseOffset;
	};

	struct GbufferPushConstants {
		uint32_t instanceBaseOffset;
		uint32_t useTaa;
		uint32_t jitterIndex;
	};


	struct LightPushConstants {
		uint32_t radius;
	};



}