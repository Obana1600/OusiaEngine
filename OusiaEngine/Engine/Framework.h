#pragma once

#include "Engine.h"


namespace ou {

class Framework {
public: // メンバ関数

	explicit Framework(EngineDesc desc);
	virtual ~Framework() = default;

	void Run();


protected: // メンバ関数

	virtual void Initialize();
	virtual void Finalize();
	virtual void Update();
	virtual void Draw();

	bool IsEndRequested() const;


protected: // メンバ変数

	Engine engine_;


private: // メンバ変数

	EngineDesc desc_;
	bool isEndRequested_ = false;
};

} // namespace ou
