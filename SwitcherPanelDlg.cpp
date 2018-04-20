/* -LICENSE-START-
** Copyright (c) 2011 Blackmagic Design
**
** Permission is hereby granted, free of charge, to any person or organization
** obtaining a copy of the software and accompanying documentation covered by
** this license (the "Software") to use, reproduce, display, distribute,
** execute, and transmit the Software, and to prepare derivative works of the
** Software, and to permit third-parties to whom the Software is furnished to
** do so, all subject to the following:
** 
** The copyright notices in the Software and this entire statement, including
** the above license grant, this restriction and the following disclaimer,
** must be included in all copies of the Software, in whole or in part, and
** all derivative works of the Software, unless such copies or derivative
** works are solely in the form of machine-executable object code generated by
** a source language processor.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
** -LICENSE-END-
*/

#include "stdafx.h"
#include "SwitcherPanel.h"
#include "SwitcherPanelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// User defined messages for signalling callback results to main thread
#define WM_MIX_EFFECT_BLOCK_PROGRAM_INPUT_CHANGED				WM_USER+1
#define WM_MIX_EFFECT_BLOCK_PREVIEW_INPUT_CHANGED				WM_USER+2
#define WM_MIX_EFFECT_BLOCK_IN_TRANSITION_CHANGED				WM_USER+3
#define WM_MIX_EFFECT_BLOCK_TRANSITION_POSITION_CHANGED			WM_USER+4
#define WM_MIX_EFFECT_BLOCK_TRANSITION_FRAMES_REMAINING_CHANGED	WM_USER+5
#define WM_MIX_EFFECT_BLOCK_FTB_FRAMES_REMAINING_CHANGED		WM_USER+6
#define WM_SWITCHER_INPUT_LONGNAME_CHANGED						WM_USER+7
#define WM_SWITCHER_DISCONNECTED								WM_USER+8

// Callback class for monitoring property changes on a mix effect block.
class MixEffectBlockMonitor : public IBMDSwitcherMixEffectBlockCallback
{
public:
	MixEffectBlockMonitor(HWND hWnd) : mHwnd(hWnd), mRefCount(1) { }

protected:
	virtual ~MixEffectBlockMonitor() { }

public:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
	{
		if (!ppv)
			return E_POINTER;

		if (IsEqualGUID(iid, IID_IBMDSwitcherMixEffectBlockCallback))
		{
			*ppv = static_cast<IBMDSwitcherMixEffectBlockCallback*>(this);
			AddRef();
			return S_OK;
		}

		if (IsEqualGUID(iid, IID_IUnknown))
		{
			*ppv = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		*ppv = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return InterlockedIncrement(&mRefCount);
	}

	ULONG STDMETHODCALLTYPE Release(void)
	{
		int newCount = InterlockedDecrement(&mRefCount);
		if (newCount == 0)
			delete this;
		return newCount;
	}

	HRESULT STDMETHODCALLTYPE PropertyChanged(BMDSwitcherMixEffectBlockPropertyId propertyId)
	{
		switch (propertyId)
		{
			case bmdSwitcherMixEffectBlockPropertyIdProgramInput:
				PostMessage(mHwnd, WM_MIX_EFFECT_BLOCK_PROGRAM_INPUT_CHANGED, 0, 0);
				break;
			case bmdSwitcherMixEffectBlockPropertyIdPreviewInput:
				PostMessage(mHwnd, WM_MIX_EFFECT_BLOCK_PREVIEW_INPUT_CHANGED, 0, 0);
				break;
			case bmdSwitcherMixEffectBlockPropertyIdInTransition:
				PostMessage(mHwnd, WM_MIX_EFFECT_BLOCK_IN_TRANSITION_CHANGED, 0, 0);
				break;
			case bmdSwitcherMixEffectBlockPropertyIdTransitionPosition:
				PostMessage(mHwnd, WM_MIX_EFFECT_BLOCK_TRANSITION_POSITION_CHANGED, 0, 0);
				break;
			case bmdSwitcherMixEffectBlockPropertyIdTransitionFramesRemaining:
				PostMessage(mHwnd, WM_MIX_EFFECT_BLOCK_TRANSITION_FRAMES_REMAINING_CHANGED, 0, 0);
				break;
			case bmdSwitcherMixEffectBlockPropertyIdFadeToBlackFramesRemaining:
				PostMessage(mHwnd, WM_MIX_EFFECT_BLOCK_FTB_FRAMES_REMAINING_CHANGED, 0, 0);
				break;
			default:	// ignore other property changes not used for this sample app
				break;
		}
		return S_OK;
	}

private:
	HWND		mHwnd;
	LONG		mRefCount;
};

// Monitor the properties on Switcher Inputs.
// In this sample app we're only interested in changes to the Long Name property to update the PopupButton list
class InputMonitor : public IBMDSwitcherInputCallback
{
public:
	InputMonitor(IBMDSwitcherInput* input, HWND hWnd) : mInput(input), mHwnd(hWnd), mRefCount(1)
	{
		mInput->AddRef();
		mInput->AddCallback(this);
	}

protected:
	~InputMonitor()
	{
		mInput->RemoveCallback(this);
		mInput->Release();
	}

public:
	// IBMDSwitcherInputCallback interface
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
	{
		if (!ppv)
			return E_POINTER;

		if (IsEqualGUID(iid, IID_IBMDSwitcherInputCallback))
		{
			*ppv = static_cast<IBMDSwitcherInputCallback*>(this);
			AddRef();
			return S_OK;
		}

		if (IsEqualGUID(iid, IID_IUnknown))
		{
			*ppv = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		*ppv = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return InterlockedIncrement(&mRefCount);
	}

	ULONG STDMETHODCALLTYPE Release(void)
	{
		int newCount = InterlockedDecrement(&mRefCount);
		if (newCount == 0)
			delete this;
		return newCount;
	}

	HRESULT STDMETHODCALLTYPE Notify(BMDSwitcherInputEventType eventType)
	{
		switch (eventType)
		{
			case bmdSwitcherInputEventTypeLongNameChanged:
				PostMessage(mHwnd, WM_SWITCHER_INPUT_LONGNAME_CHANGED, 0, 0);
				break;
			default:	// ignore other property changes not used for this sample app
				break;
		}
		return S_OK;
	}
	IBMDSwitcherInput* input() { return mInput; }

private:
	IBMDSwitcherInput*		mInput;
	HWND					mHwnd;
	LONG					mRefCount;
};

// Callback class to monitor switcher disconnection
class SwitcherMonitor : public IBMDSwitcherCallback
{
public:
	SwitcherMonitor(HWND hWnd) :	mHwnd(hWnd), mRefCount(1) { }

protected:
	virtual ~SwitcherMonitor() { }

public:
	// IBMDSwitcherCallback interface
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
	{
		if (!ppv)
			return E_POINTER;

		if (IsEqualGUID(iid, IID_IBMDSwitcherCallback))
		{
			*ppv = static_cast<IBMDSwitcherCallback*>(this);
			AddRef();
			return S_OK;
		}

		if (IsEqualGUID(iid, IID_IUnknown))
		{
			*ppv = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}

		*ppv = NULL;
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return InterlockedIncrement(&mRefCount);
	}

	ULONG STDMETHODCALLTYPE Release(void)
	{
		int newCount = InterlockedDecrement(&mRefCount);
		if (newCount == 0)
			delete this;
		return newCount;
	}

	// Switcher event callback
	HRESULT STDMETHODCALLTYPE	Notify(BMDSwitcherEventType eventType, BMDSwitcherVideoMode coreVideoMode)
	{
		if (eventType == bmdSwitcherEventTypeDisconnected)
		{
			PostMessage(mHwnd, WM_SWITCHER_DISCONNECTED, 0, 0);
		}
		
		return S_OK;
	}

private:
	HWND	mHwnd;
	LONG	mRefCount;
};


// CSwitcherPanelDlg dialog

CSwitcherPanelDlg::CSwitcherPanelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSwitcherPanelDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSwitcherPanelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EDIT_ADDRESS, mEditAddress);
	DDX_Control(pDX, IDC_BUTTON_CONNECT, mButtonConnect);
	DDX_Control(pDX, IDC_EDIT_NAME, mEditName);
	DDX_Control(pDX, IDC_COMBO_PROGRAM, mComboProgram);
	DDX_Control(pDX, IDC_COMBO_PREVIEW, mComboPreview);
	DDX_Control(pDX, IDC_SLIDER, mSlider);
	DDX_Control(pDX, IDC_BUTTON_AUTO, mButtonAuto);
	DDX_Control(pDX, IDC_BUTTON_CUT, mButtonCut);
	DDX_Control(pDX, IDC_BUTTON_FTB, mButtonFTB);
	DDX_Control(pDX, IDC_EDIT_TRANSITION_FRAMES, mEditTransitionFrames);
	DDX_Control(pDX, IDC_EDIT_FTB_FRAMES, mEditFTBFrames);
}

BEGIN_MESSAGE_MAP(CSwitcherPanelDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_VSCROLL()				// The slider's VSCROLL messages are handled in OnVScroll
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CSwitcherPanelDlg::OnBnClickedConnect)
	ON_BN_CLICKED(IDC_BUTTON_AUTO, &CSwitcherPanelDlg::OnBnClickedAuto)
	ON_BN_CLICKED(IDC_BUTTON_CUT, &CSwitcherPanelDlg::OnBnClickedCut)
	ON_BN_CLICKED(IDC_BUTTON_FTB, &CSwitcherPanelDlg::OnBnClickedFTB)
	ON_CBN_SELENDOK(IDC_COMBO_PROGRAM, &CSwitcherPanelDlg::OnProgramInputChanged)
	ON_CBN_SELENDOK(IDC_COMBO_PREVIEW, &CSwitcherPanelDlg::OnPreviewInputChanged)

	ON_MESSAGE(WM_MIX_EFFECT_BLOCK_PROGRAM_INPUT_CHANGED, OnMixEffectBlockProgramInputChanged)
	ON_MESSAGE(WM_MIX_EFFECT_BLOCK_PREVIEW_INPUT_CHANGED, OnMixEffectBlockPreviewInputChanged)
	ON_MESSAGE(WM_MIX_EFFECT_BLOCK_IN_TRANSITION_CHANGED, OnMixEffectBlockInTransitionChanged)
	ON_MESSAGE(WM_MIX_EFFECT_BLOCK_TRANSITION_POSITION_CHANGED, OnMixEffectBlockTransitionPositionChanged)
	ON_MESSAGE(WM_MIX_EFFECT_BLOCK_TRANSITION_FRAMES_REMAINING_CHANGED, OnMixEffectBlockTransitionFramesRemainingChanged)
	ON_MESSAGE(WM_MIX_EFFECT_BLOCK_FTB_FRAMES_REMAINING_CHANGED, OnMixEffectBlockFTBFramesRemainingChanged)
	ON_MESSAGE(WM_SWITCHER_INPUT_LONGNAME_CHANGED, OnSwitcherInputLongnameChanged)
	ON_MESSAGE(WM_SWITCHER_DISCONNECTED, OnSwitcherDisconnected)
END_MESSAGE_MAP()


// CSwitcherPanelDlg message handlers

BOOL CSwitcherPanelDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog. The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Initialize COM and Switcher related members
	if (FAILED(CoInitialize(NULL)))
	{
		MessageBox(_T("CoInitialize failed."), _T("Error"));
		goto bail;
	}

	mSwitcherDiscovery = NULL;
	mSwitcher = NULL;
	mMixEffectBlock = NULL;

	mSwitcherMonitor = new SwitcherMonitor(m_hWnd);
	mMixEffectBlockMonitor = new MixEffectBlockMonitor(m_hWnd);

	mMoveSliderDownwards = FALSE;
	mCurrentTransitionReachedHalfway = FALSE;

	mSwitcherDiscovery = NULL;
	HRESULT hr = CoCreateInstance(CLSID_CBMDSwitcherDiscovery, NULL, CLSCTX_ALL, IID_IBMDSwitcherDiscovery, (void**)&mSwitcherDiscovery);
	if (FAILED(hr))
	{
		MessageBox(_T("Could not create Switcher Discovery Instance.\nATEM Switcher Software may not be installed."), _T("Error"));
		goto bail;
	}

	switcherDisconnected();		// start with switcher disconnected

	return TRUE;				// return TRUE unless you set the focus to a control

bail:
	return FALSE;
}

void CSwitcherPanelDlg::OnBnClickedConnect()
{
	BMDSwitcherConnectToFailure			failReason;
	CString address;

	mEditAddress.GetWindowText(address);

	BSTR addressBstr = address.AllocSysString();

	// Note that ConnectTo() can take several seconds to return, both for success or failure,
	// depending upon hostname resolution and network response times, so it may be best to
	// do this in a separate thread to prevent the main GUI thread blocking.
	HRESULT hr = mSwitcherDiscovery->ConnectTo(addressBstr, &mSwitcher, &failReason);
	SysFreeString(addressBstr);
	if (SUCCEEDED(hr))
	{
		switcherConnected();
	}
	else
	{
		switch (failReason)
		{
			case bmdSwitcherConnectToFailureNoResponse:
				MessageBox(_T("No response from Switcher"), _T("Error"));
				break;
			case bmdSwitcherConnectToFailureIncompatibleFirmware:
				MessageBox(_T("Switcher has incompatible firmware"), _T("Error"));
				break;
			default:
				MessageBox(_T("Connection failed for unknown reason"), _T("Error"));
				break;
		}
	}
}

void CSwitcherPanelDlg::OnBnClickedAuto()
{
	mMixEffectBlock->PerformAutoTransition();
}

void CSwitcherPanelDlg::OnBnClickedCut()
{
	mMixEffectBlock->PerformCut();
}

void CSwitcherPanelDlg::OnBnClickedFTB()
{
	mMixEffectBlock->PerformFadeToBlack();
}

void CSwitcherPanelDlg::OnProgramInputChanged()
{
	BMDSwitcherInputId inputId = mComboProgram.GetItemData(mComboProgram.GetCurSel());

	mMixEffectBlock->SetInt(bmdSwitcherMixEffectBlockPropertyIdProgramInput, inputId);
}

void CSwitcherPanelDlg::OnPreviewInputChanged()
{
	BMDSwitcherInputId inputId = mComboPreview.GetItemData(mComboPreview.GetCurSel());
	mMixEffectBlock->SetInt(bmdSwitcherMixEffectBlockPropertyIdPreviewInput, inputId);
}

void CSwitcherPanelDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// Handle only the Slider events
	if (nSBCode != SB_THUMBTRACK && nSBCode != SB_THUMBPOSITION)
		return;

	double position = nPos / 100.0;			// convert to [0 .. 1] range
	if (mMoveSliderDownwards)
		position = (100 - nPos) / 100.0;	// deal with flipped slider handle position

	mMixEffectBlock->SetFloat(bmdSwitcherMixEffectBlockPropertyIdTransitionPosition, position);
}

void CSwitcherPanelDlg::switcherConnected()
{
	HRESULT result;
	IBMDSwitcherMixEffectBlockIterator* iterator = NULL;
	IBMDSwitcherInputIterator* inputIterator = NULL;

	mButtonConnect.EnableWindow(FALSE);	// disable Connect button while connected

	BSTR productName;
	if (FAILED(mSwitcher->GetProductName(&productName)))
	{
		TRACE(_T("Could not get switcher product name"));
		return;
	}
	CString productNameCString(productName);
	SysFreeString(productName);
	mEditName.SetWindowText(productNameCString);

	mSwitcher->AddCallback(mSwitcherMonitor);

	// Create an InputMonitor for each input so we can catch any changes to input names
	result = mSwitcher->CreateIterator(IID_IBMDSwitcherInputIterator, (void**)&inputIterator);
	if (SUCCEEDED(result))
	{
		IBMDSwitcherInput* input = NULL;

		// For every input, install a callback to monitor property changes on the input
		while (S_OK == inputIterator->Next(&input))
		{
			InputMonitor* inputMonitor = new InputMonitor(input, m_hWnd);
			input->Release();
			mInputMonitors.push_back(inputMonitor);
		}
		inputIterator->Release();
		inputIterator = NULL;
	}

	// Get the mix effect block iterator
	result = mSwitcher->CreateIterator(IID_IBMDSwitcherMixEffectBlockIterator, (void**)&iterator);
	if (FAILED(result))
	{
		TRACE(_T("Could not create IBMDSwitcherMixEffectBlockIterator iterator"));
		goto finish;
	}

	// Use the first Mix Effect Block
	if (S_OK != iterator->Next(&mMixEffectBlock))
	{
		TRACE(_T("Could not get the first IBMDSwitcherMixEffectBlock"));
		goto finish;
	}

	mMixEffectBlock->AddCallback(mMixEffectBlockMonitor);

	mixEffectBlockBoxSetEnabled(TRUE);
	updatePopupButtonItems();
	updateSliderPosition();
	updateTransitionFramesText();
	updateFTBFramesText();

finish:
	if (iterator)
		iterator->Release();
}

void CSwitcherPanelDlg::switcherDisconnected()
{
	mButtonConnect.EnableWindow(TRUE);		// enable connect button so user can re-connect
	mEditName.SetWindowText(_T(""));

	mixEffectBlockBoxSetEnabled(FALSE);

	// cleanup resources created when switcher was connected
	for (std::list<InputMonitor*>::iterator it = mInputMonitors.begin(); it != mInputMonitors.end(); ++it)
	{
		(*it)->Release();
	}
	mInputMonitors.clear();

	if (mMixEffectBlock)
	{
		mMixEffectBlock->RemoveCallback(mMixEffectBlockMonitor);
		mMixEffectBlock->Release();
		mMixEffectBlock = NULL;
	}

	if (mSwitcher)
	{
		mSwitcher->RemoveCallback(mSwitcherMonitor);
		mSwitcher->Release();
		mSwitcher = NULL;
	}
}

//
// GUI updates
//
void CSwitcherPanelDlg::updatePopupButtonItems()
{
	HRESULT result;
	IBMDSwitcherInputIterator* inputIterator = NULL;
	IBMDSwitcherInput* input = NULL;

	result = mSwitcher->CreateIterator(IID_IBMDSwitcherInputIterator, (void**)&inputIterator);
	if (FAILED(result))
	{
		TRACE(_T("Could not create IBMDSwitcherInputIterator iterator"));
		return;
	}

	mComboProgram.ResetContent();
	mComboPreview.ResetContent();

	BSTR longName;
	while (S_OK == inputIterator->Next(&input))
	{
		BMDSwitcherInputId id;
		int newIndex;

		input->GetInputId(&id);
		input->GetLongName(&longName);
		CString longNameCString(longName);
		SysFreeString(longName);

		newIndex = mComboProgram.AddString(longNameCString);
		mComboProgram.SetItemData(newIndex, (DWORD_PTR)id);

		newIndex = mComboPreview.AddString(longNameCString);
		mComboPreview.SetItemData(newIndex, (DWORD_PTR)id);

		input->Release();
	}
	inputIterator->Release();

	updateProgramButtonSelection();
	updatePreviewButtonSelection();
}

void CSwitcherPanelDlg::updateProgramButtonSelection()
{
	BMDSwitcherInputId	programId;
	mMixEffectBlock->GetInt(bmdSwitcherMixEffectBlockPropertyIdProgramInput, &programId);

	for (int i = 0; i < mComboProgram.GetCount(); i++)
	{
		if (mComboProgram.GetItemData(i) == programId)
		{
			mComboProgram.SetCurSel(i);
			break;
		}
	}
}

void CSwitcherPanelDlg::updatePreviewButtonSelection()
{
	BMDSwitcherInputId	previewId;
	mMixEffectBlock->GetInt(bmdSwitcherMixEffectBlockPropertyIdPreviewInput, &previewId);

	for (int i = 0; i < mComboPreview.GetCount(); i++)
	{
		if (mComboPreview.GetItemData(i) == previewId)
		{
			mComboPreview.SetCurSel(i);
			break;
		}
	}
}

void CSwitcherPanelDlg::updateSliderPosition()
{
	double position;
	mMixEffectBlock->GetFloat(bmdSwitcherMixEffectBlockPropertyIdTransitionPosition, &position);

	// Record when transition passes halfway so we can flip orientation of slider handle at the end of transition
	mCurrentTransitionReachedHalfway = (position >= 0.50);

	double sliderPosition = position * 100;
	if (mMoveSliderDownwards)
		sliderPosition = 100 - position * 100;		// slider handle moving in opposite direction

	int positionRoundedToInt = (int)(sliderPosition + 0.5);
	mSlider.SetPos(positionRoundedToInt);
}

void CSwitcherPanelDlg::updateTransitionFramesText()
{
	long long framesRemaining;
	mMixEffectBlock->GetInt(bmdSwitcherMixEffectBlockPropertyIdTransitionFramesRemaining, &framesRemaining);
	CString framesStr;
	framesStr.Format(_T("%u"), framesRemaining);
	mEditTransitionFrames.SetWindowTextW(framesStr);
}

void CSwitcherPanelDlg::updateFTBFramesText()
{
	long long framesRemaining;
	mMixEffectBlock->GetInt(bmdSwitcherMixEffectBlockPropertyIdFadeToBlackFramesRemaining, &framesRemaining);
	CString framesStr;
	framesStr.Format(_T("%u"), framesRemaining);
	mEditFTBFrames.SetWindowText(framesStr);
}

LRESULT CSwitcherPanelDlg::OnMixEffectBlockProgramInputChanged(WPARAM wParam, LPARAM lParam)
{
	updateProgramButtonSelection();
	return 0;
}

LRESULT CSwitcherPanelDlg::OnMixEffectBlockPreviewInputChanged(WPARAM wParam, LPARAM lParam)
{
	updatePreviewButtonSelection();
	return 0;
}

LRESULT CSwitcherPanelDlg::OnMixEffectBlockInTransitionChanged(WPARAM wParam, LPARAM lParam)
{
	BOOL inTransition;
	mMixEffectBlock->GetFlag(bmdSwitcherMixEffectBlockPropertyIdInTransition, &inTransition);

	if (inTransition == FALSE)
	{
		// Toggle the starting orientation of slider handle if a transition has passed through halfway
		if (mCurrentTransitionReachedHalfway)
		{
			mMoveSliderDownwards = ! mMoveSliderDownwards;
			updateSliderPosition();
		}

		mCurrentTransitionReachedHalfway = FALSE;
	}

	return 0;
}

LRESULT CSwitcherPanelDlg::OnMixEffectBlockTransitionPositionChanged(WPARAM wParam, LPARAM lParam)
{
	updateSliderPosition();
	return 0;
}

LRESULT CSwitcherPanelDlg::OnMixEffectBlockTransitionFramesRemainingChanged(WPARAM wParam, LPARAM lParam)
{
	updateTransitionFramesText();
	return 0;
}

LRESULT CSwitcherPanelDlg::OnMixEffectBlockFTBFramesRemainingChanged(WPARAM wParam, LPARAM lParam)
{
	updateFTBFramesText();
	return 0;
}

LRESULT CSwitcherPanelDlg::OnSwitcherInputLongnameChanged(WPARAM wParam, LPARAM lParam)
{
	updatePopupButtonItems();
	return 0;
}

LRESULT CSwitcherPanelDlg::OnSwitcherDisconnected(WPARAM wParam, LPARAM lParam)
{
	switcherDisconnected();
	return 0;
}

void CSwitcherPanelDlg::mixEffectBlockBoxSetEnabled(BOOL enabled)
{
	mComboProgram.EnableWindow(enabled);
	mComboPreview.EnableWindow(enabled);
	mSlider.EnableWindow(enabled);
	mButtonAuto.EnableWindow(enabled);
	mButtonCut.EnableWindow(enabled);
	mButtonFTB.EnableWindow(enabled);
	mEditTransitionFrames.EnableWindow(enabled);
	mEditFTBFrames.EnableWindow(enabled);
}
