package com.attackercommunity.acdcbadge;

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AlertDialog;
import android.text.InputType;
import android.widget.EditText;

public class NameChangeDialog extends DialogFragment {

    private EditText titleInput;
    private BadgeEditNameListener listener;

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        builder.setTitle(R.string.change_title);
        titleInput = new EditText(getContext());
        titleInput.setInputType(InputType.TYPE_CLASS_TEXT);
        try {
            listener = (BadgeEditNameListener) getActivity();
        } catch(ClassCastException ex) {
            throw new ClassCastException(getActivity().toString() +
                    "must implement BadgeEditNameListener!");
        }
        titleInput.setText(listener.getCurrentName());
        builder.setView(titleInput);
        builder.setPositiveButton(R.string.change, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                listener.setNewName(titleInput.getText().toString());
            }
        });
        builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.cancel();
            }
        });
        return builder.create();
    }

    public interface BadgeEditNameListener {
        String getCurrentName();
        void setNewName(String name);
    }
}
