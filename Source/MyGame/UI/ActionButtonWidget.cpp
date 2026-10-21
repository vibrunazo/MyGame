#include "ActionButtonWidget.h"
#include "../Player/MyPlayerController.h"

#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "UObject/EnumProperty.h"
#include "UObject/UnrealType.h"

void UActionButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	const AMyPlayerController* Controller = GetOwningPlayer<AMyPlayerController>();
	ShowInputDevice(!Controller || Controller->IsUsingGamepad());
}

void UActionButtonWidget::ShowInputDevice(bool bGamepad)
{
	const AMyPlayerController* Controller = GetOwningPlayer<AMyPlayerController>();
	const FText Keys = Controller ? Controller->GetKeyboardText(GetSlot()) : FText::GetEmpty();
	// a slot no key presses keeps the gamepad picture
	const bool bShowKeys = !bGamepad && !Keys.IsEmpty() && CommandTextWidget;
	if (bShowKeys)
	{
		CommandTextWidget->SetText(Keys);
		CommandTextWidget->SetIsEnabled(true);
		CommandTextWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (HotkeySwitcher) HotkeySwitcher->SetActiveWidgetIndex(bShowKeys ? 1 : 0);
}

EInput UActionButtonWidget::GetSlot() const
{
	static const FName SlotName(TEXT("ButtonInput"));
	const FProperty* Property = GetClass()->FindPropertyByName(SlotName);
	if (const FByteProperty* Byte = CastField<FByteProperty>(Property))
	{
		return (EInput)Byte->GetPropertyValue_InContainer(this);
	}
	if (const FEnumProperty* Enum = CastField<FEnumProperty>(Property))
	{
		return (EInput)Enum->GetUnderlyingProperty()->GetUnsignedIntPropertyValue(Enum->ContainerPtrToValuePtr<void>(this));
	}
	return EInput::None;
}
