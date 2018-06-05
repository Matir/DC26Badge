package com.attackercommunity.acdcbadge;

import android.support.constraint.ConstraintLayout;
import android.support.v7.widget.RecyclerView;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Spanned;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.Spinner;

import java.util.ArrayList;
import java.util.List;

public class MessageListAdapter extends RecyclerView.Adapter<MessageListAdapter.ViewHolder> {
    private static final String TAG = "MessageListAdapter";
    private static final List<MessageMode> modeOptions = MessageMode.asList();
    private static final List<MessageSpeed> speedOptions = MessageSpeed.asList();

    private final IActiveMessageChanger messageChanger;
    private final List<BLEBadge.BLEBadgeMessage> messageSet = new ArrayList<>();
    private int checkedPosition = 0;

    public class ViewHolder extends RecyclerView.ViewHolder {

        private final ConstraintLayout mLayout;
        private final RadioButton mSelectButton;
        private final EditText mMessageText;
        private final Spinner mStyleSpinner;
        private final Spinner mSpeedSpinner;

        private BLEBadge.BLEBadgeMessage mMessage = null;

        public ViewHolder(ConstraintLayout view) {
            super(view);
            mLayout = view;
            mSelectButton = view.findViewById(R.id.message_select_btn);
            mMessageText = view.findViewById(R.id.message_text);
            mStyleSpinner = view.findViewById(R.id.style_spinner);
            mSpeedSpinner = view.findViewById(R.id.speed_spinner);

            mSelectButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    MessageListAdapter.this.checkedPosition = getAdapterPosition();
                    MessageListAdapter.this.messageChanger.onActiveMessageChanged(
                            MessageListAdapter.this.checkedPosition);
                    MessageListAdapter.this.notifyDataSetChanged();
                }
            });

            ArrayAdapter<MessageMode> styleAdapter = new ArrayAdapter<>(
                    mLayout.getContext(),
                    android.R.layout.simple_spinner_item,
                    modeOptions);
            styleAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            mStyleSpinner.setAdapter(styleAdapter);
            mStyleSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                    mMessage.setMode((MessageMode)mStyleSpinner.getSelectedItem());
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {

                }
            });

            ArrayAdapter<MessageSpeed> speedAdapter = new ArrayAdapter<>(
                    mLayout.getContext(),
                    android.R.layout.simple_spinner_item,
                    speedOptions);
            speedAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
            mSpeedSpinner.setAdapter(speedAdapter);
            mSpeedSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                    mMessage.setSpeed((MessageSpeed)mSpeedSpinner.getSelectedItem());
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {

                }
            });

            // Limit input length
            mMessageText.setFilters(new InputFilter[]{
                    new InputFilter.LengthFilter(Constants.MessageMaxLength)});
            mMessageText.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                    // Don't care
                }

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    // Don't care
                }

                @Override
                public void afterTextChanged(Editable s) {
                    try {
                        mMessage.setText(s.toString());
                    } catch (BLEBadge.BLEBadgeException ex) {
                        // Really should display an error, but we do have the filter.
                        Log.e(TAG, "Message length too long!", ex);
                    }
                }
            });
        }

        public void UpdateMessage(BLEBadge.BLEBadgeMessage message) {
            mMessage = message;
            mMessageText.setText(message.getText());
            mStyleSpinner.setSelection(modeOptions.indexOf(message.getMode()));
            mSpeedSpinner.setSelection(speedOptions.indexOf(message.getSpeed()));
        }
    }

    public MessageListAdapter(IActiveMessageChanger changer) {
        super();
        messageChanger = changer;
    }

    public void setMessages(List<BLEBadge.BLEBadgeMessage> messages) {
        synchronized (this) {
            messageSet.clear();
            messageSet.addAll(messages);
        }
        notifyDataSetChanged();
    }

    public void setCheckedPosition(int position) {
        if (position < 0 || position > getItemCount())
            return;
        checkedPosition = position;
        notifyDataSetChanged();
    }

    @Override
    public MessageListAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        ConstraintLayout layout = (ConstraintLayout) LayoutInflater.from(parent.getContext())
                .inflate(R.layout.message_setup, parent, false);
        return new ViewHolder(layout);
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        if (position < 0 || position > messageSet.size()) {
            Log.e(TAG, "Received a position outside the messageSet!");
            return;
        }

        final BLEBadge.BLEBadgeMessage message = messageSet.get(position);
        holder.UpdateMessage(message);

        // Update checked status
        holder.mSelectButton.setChecked(position == checkedPosition);
    }

    @Override
    public int getItemCount() {
        return messageSet.size();
    }

    interface IActiveMessageChanger {
        void onActiveMessageChanged(int pos);
    }
}
