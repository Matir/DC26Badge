package com.attackercommunity.acdcbadge;

import android.support.constraint.ConstraintLayout;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import android.widget.Spinner;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

public class MessageListAdapter extends RecyclerView.Adapter<MessageListAdapter.ViewHolder> {
    private static final String TAG = "MessageListAdapter";
    private final List<BLEBadge.BLEBadgeMessage> messageSet = new ArrayList<>();
    private int checkedPosition = 0;


    public class ViewHolder extends RecyclerView.ViewHolder {
        private final ConstraintLayout mLayout;
        private final RadioButton mSelectButton;
        private final TextView mMessageText;
        private final Spinner mStyleSpinner;
        private final Spinner mSpeedSpinner;

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
                    MessageListAdapter.this.notifyDataSetChanged();
                }
            });
        }
    }

    public MessageListAdapter() {}

    @Override
    public MessageListAdapter.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        ConstraintLayout layout = (ConstraintLayout) LayoutInflater.from(parent.getContext())
                .inflate(R.layout.message_setup, parent, false);
        return new ViewHolder(layout);
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {

        // Update checked status
        holder.mSelectButton.setChecked(position == checkedPosition);
    }

    @Override
    public int getItemCount() {
        return messageSet.size();
    }
}
