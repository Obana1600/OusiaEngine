#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>


namespace ou {

class EditorWindow {
	friend class EditorMainWindow;
	friend class Engine;

public: // 静的メンバ関数

	template <typename T>
	static T *GetWindow();


public: // メンバ関数

	virtual ~EditorWindow() = default;

	void Show();
	void Close();
	void Focus();

	void SetTitle(std::string_view title);
	const std::string &GetTitle() const;


protected: // メンバ関数

	EditorWindow() = default;

	virtual void OnGui() = 0;


private: // 静的メンバ変数

	static inline std::vector<std::unique_ptr<EditorWindow>> sWindows_;


private: // メンバ変数

	std::string title_;
	bool isOpen_ = true;
	bool wantsFocus_ = false;


private: // 静的メンバ関数

	static void DrawAll();
	static void DestroyAll();
};



template <typename T>
inline T *EditorWindow::GetWindow() {
	static_assert(std::is_base_of_v<EditorWindow, T>, "T は EditorWindow の派生クラスでなければいけません");

	for (const auto &window : sWindows_) {
		if (auto *found = dynamic_cast<T *>(window.get())) {
			found->Show();
			found->Focus();
			return found;
		}
	}

	auto window = std::make_unique<T>();
	auto *ptr = window.get();
	sWindows_.push_back(std::move(window));
	return ptr;
}

} // namespace ou
