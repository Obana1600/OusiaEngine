#pragma once

#include "Math/Vector3.h"
#include "Scene/Behaviour.h"
#include "UI/ColorBlock.h"
#include "UI/EventSystem/EventInterfaces.h"

#include <cstdint>
#include <vector>


namespace ou {

// 前方宣言
class Graphic;

class Selectable: public Behaviour,
				  public IPointerEnterHandler,
				  public IPointerExitHandler,
				  public IPointerDownHandler,
				  public IPointerUpHandler,
				  public ISelectHandler,
				  public IDeselectHandler,
				  public IMoveHandler {
public: // 静的メンバ関数

	static const std::vector<Selectable *> &GetAllSelectables();


public: // メンバ関数

	void Awake() override;
	void OnEnable() override;
	void OnDisable() override;
	void OnDestroy() override;

	void SetInteractable(bool interactable);
	bool IsInteractable() const;

	void SetTargetGraphic(Graphic *targetGraphic);
	Graphic *GetTargetGraphic() const;

	void SetColors(const ColorBlock &colors);
	const ColorBlock &GetColors() const;

	Selectable *FindSelectable(const Vector3 &dir) const;
	Selectable *FindSelectableOnLeft() const;
	Selectable *FindSelectableOnRight() const;
	Selectable *FindSelectableOnUp() const;
	Selectable *FindSelectableOnDown() const;

	void Select();

	void OnPointerEnter(PointerEventData &eventData) override;
	void OnPointerExit(PointerEventData &eventData) override;
	void OnPointerDown(PointerEventData &eventData) override;
	void OnPointerUp(PointerEventData &eventData) override;
	void OnSelect(BaseEventData &eventData) override;
	void OnDeselect(BaseEventData &eventData) override;
	void OnMove(AxisEventData &eventData) override;


protected: // インナークラス

	enum class SelectionState : uint8_t {
		Normal,
		Highlighted,
		Pressed,
		Selected,
		Disabled,
	};


protected: // メンバ関数

	SelectionState GetCurrentSelectionState() const;

	virtual void DoStateTransition(SelectionState state, bool instant);


private: // メンバ変数

	Graphic *targetGraphic_ = nullptr;
	ColorBlock colors_{};
	bool interactable_ = true;

	bool isPointerInside_ = false;
	bool isPointerDown_ = false;
	bool hasSelection_ = false;


private: // 静的メンバ変数

	static inline std::vector<Selectable *> sSelectables_;


private: // メンバ関数

	void EvaluateAndTransitionToSelectionState();

	Vector3 CalculateWorldCenter() const;
};

} // namespace ou
